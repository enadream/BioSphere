#include "world/chunk_streamer.hpp"
#include "core/log.hpp"

#include <algorithm>
#include <cmath>
#include <exception>

// ---Construction / destruction ---

ChunkStreamer::ChunkStreamer(const Config& cfg)
    : m_Generator(cfg.seed)
{
    m_RenderDist = cfg.renderDistance;
    m_LoadDist   = cfg.loadDistance > 0
        ? cfg.loadDistance
        : static_cast<uint32_t>(std::ceil(cfg.renderDistance * LOAD_DISTANCE_FACTOR));

    m_WorldDir   = cfg.worldDir;
    m_RegionsDir = cfg.worldDir + "/regions";

    // Use unbounded cache (cap=0) so LRU never evicts chunks inside the load ring.
    // EvictFarChunks is the sole eviction path - chunks are only saved when they
    // genuinely leave the load distance, not due to arbitrary LRU pressure.
    const size_t cap = cfg.cacheCapacity; // 0 = unbounded

    m_Cache = ChunkCache(cap);

    const uint32_t workers = std::max(1u, cfg.workerCount);
    m_Workers.reserve(workers);
    m_Threads.reserve(workers);

    for (uint32_t i = 0; i < workers; ++i) {
        m_Workers.emplace_back(std::make_unique<WorkerState>());
        m_Threads.emplace_back(&ChunkStreamer::WorkerLoop, this, i);
    }
}

ChunkStreamer::~ChunkStreamer() {
    m_Running.store(false);
    for (auto& ws : m_Workers)
        ws->cv.notify_all();
    for (auto& t : m_Threads)
        if (t.joinable()) t.join();
}

// ---Public API ---

bool ChunkStreamer::Tick(ChunkCoordinates center) {
    bool changed = DrainCompleted();

    if (!m_TickCenterValid || center != m_LastTickCenter) {
        const ChunkCoordinates prevCenter = m_LastTickCenter;
        const bool firstTick = !m_TickCenterValid;
        m_TickCenterValid = true;
        m_LastTickCenter  = center;

        RequestLoadRing(prevCenter, center, firstTick);
        if (!firstTick) EvictFarChunks(prevCenter, center);

        changed = true;
    }

    return changed;
}

void ChunkStreamer::FlushAll() {
    // Drain any cached dirty chunks to disk on the main thread (workers may be busy
    // generating new chunks; running this here avoids stalling for save backlog).
    m_Cache.ForEach([this](Chunk& chunk) {
        if (!chunk.IsDirty) return;
        const ChunkCoordinates coords      = chunk.GetCoordinates();
        const ChunkCoordinates regionCoords = RegionHandler::ChunkToRegion(coords);
        const uint64_t regionID = RegionHandler::MakeID(regionCoords.X, regionCoords.Z);
        auto sr = GetOrLoadRegion(regionID, regionCoords);
        std::lock_guard<std::mutex> lock(sr->mtx);
        if (!sr->region.WriteChunk(chunk))
            LOG_ERROR("[ChunkStreamer] FlushAll: failed to write chunk (%d, %d)",
                      coords.X, coords.Z);
        chunk.IsDirty = false;
    });

    // Finally write each loaded region's header to disk (no-op if not modified).
    std::lock_guard<std::mutex> guard(m_SharedRegionsMutex);
    for (auto& [id, sr] : m_SharedRegions) {
        std::lock_guard<std::mutex> lock(sr->mtx);
        sr->region.Unload();
    }
    m_SharedRegions.clear();
}

Chunk* ChunkStreamer::GetChunk(ChunkCoordinates coords) {
    return m_Cache.Find(coords);
}

const Chunk* ChunkStreamer::GetChunk(ChunkCoordinates coords) const {
    return m_Cache.Find(coords);
}

// ---Internal helpers ---

void ChunkStreamer::RequestLoadRing(ChunkCoordinates prevCenter, ChunkCoordinates newCenter, bool firstTime) {
    const int32_t ld = static_cast<int32_t>(m_LoadDist);

    struct Pending {
        ChunkCoordinates coords;
        int32_t          distSq;
    };
    std::vector<Pending> pending;

    auto consider = [&](int32_t x, int32_t z) {
        ChunkCoordinates coords(x, z);
        const uint64_t key = ChunkCache::MakeKey(coords);
        if (m_Requested.count(key)) return;
        if (m_Cache.Contains(coords)) return;
        const int32_t dx = x - newCenter.X;
        const int32_t dz = z - newCenter.Z;
        pending.push_back({coords, dx * dx + dz * dz});
    };

    const int32_t newXMin = newCenter.X - ld, newXMax = newCenter.X + ld;
    const int32_t newZMin = newCenter.Z - ld, newZMax = newCenter.Z + ld;

    if (firstTime
        || std::abs(newCenter.X - prevCenter.X) > 2 * ld
        || std::abs(newCenter.Z - prevCenter.Z) > 2 * ld) {
        // Full ring iteration on first tick or when there is no overlap.
        pending.reserve(static_cast<size_t>((2 * ld + 1)) * static_cast<size_t>((2 * ld + 1)));
        for (int32_t z = newZMin; z <= newZMax; ++z)
            for (int32_t x = newXMin; x <= newXMax; ++x)
                consider(x, z);
    } else {
        // Leading-edge strips only - bounded by the distance moved, not the full ring.
        const int32_t prevXMin = prevCenter.X - ld, prevXMax = prevCenter.X + ld;
        const int32_t prevZMin = prevCenter.Z - ld, prevZMax = prevCenter.Z + ld;

        if (newXMin < prevXMin) {
            for (int32_t x = newXMin; x < prevXMin; ++x)
                for (int32_t z = newZMin; z <= newZMax; ++z) consider(x, z);
        }
        if (newXMax > prevXMax) {
            for (int32_t x = prevXMax + 1; x <= newXMax; ++x)
                for (int32_t z = newZMin; z <= newZMax; ++z) consider(x, z);
        }
        const int32_t overlapXMin = std::max(newXMin, prevXMin);
        const int32_t overlapXMax = std::min(newXMax, prevXMax);
        if (newZMin < prevZMin) {
            for (int32_t z = newZMin; z < prevZMin; ++z)
                for (int32_t x = overlapXMin; x <= overlapXMax; ++x) consider(x, z);
        }
        if (newZMax > prevZMax) {
            for (int32_t z = prevZMax + 1; z <= newZMax; ++z)
                for (int32_t x = overlapXMin; x <= overlapXMax; ++x) consider(x, z);
        }
    }

    std::sort(pending.begin(), pending.end(),
              [](const Pending& a, const Pending& b) { return a.distSq < b.distSq; });

    for (const Pending& p : pending) {
        const uint64_t key = ChunkCache::MakeKey(p.coords);
        m_Requested.insert(key);

        const uint32_t wi = m_NextWorker % static_cast<uint32_t>(m_Workers.size());
        m_NextWorker = (m_NextWorker + 1) % static_cast<uint32_t>(m_Workers.size());

        const ChunkCoordinates rc = RegionHandler::ChunkToRegion(p.coords);
        const uint64_t regionID = RegionHandler::MakeID(rc.X, rc.Z);

        {
            std::unique_lock<std::mutex> lock(m_Workers[wi]->mtx);
            m_Workers[wi]->loadQueue.push(LoadTask{p.coords, regionID});
        }
        m_Workers[wi]->cv.notify_one();
    }
}

bool ChunkStreamer::DrainCompleted() {
    bool changed = false;
    for (auto& ws : m_Workers) {
        std::vector<std::unique_ptr<Chunk>> batch;
        {
            std::unique_lock<std::mutex> lock(ws->mtx);
            std::swap(batch, ws->completed);
        }

        for (auto& chunkPtr : batch) {
            const ChunkCoordinates coords = chunkPtr->GetCoordinates();
            const uint64_t         key    = ChunkCache::MakeKey(coords);
            m_Cache.Insert(std::move(chunkPtr)); // unbounded cache - never evicts
            m_Requested.erase(key);
            m_RecentlyArrived.push_back(coords);
            changed = true;
        }
    }
    return changed;
}

std::vector<ChunkCoordinates> ChunkStreamer::ConsumeRecentlyArrived() {
    std::vector<ChunkCoordinates> result;
    std::swap(result, m_RecentlyArrived);
    return result;
}

bool ChunkStreamer::EvictFarChunks(ChunkCoordinates prevCenter, ChunkCoordinates newCenter) {
    const int32_t ld = static_cast<int32_t>(m_LoadDist);

    std::vector<ChunkCoordinates> toEvict;

    auto consider = [&](int32_t x, int32_t z) {
        ChunkCoordinates coords(x, z);
        if (m_Cache.Contains(coords)) toEvict.push_back(coords);
    };

    if (std::abs(newCenter.X - prevCenter.X) > 2 * ld
        || std::abs(newCenter.Z - prevCenter.Z) > 2 * ld) {
        // No overlap - the whole cache is now outside the new load ring.
        m_Cache.ForEach([&](Chunk& chunk) {
            const ChunkCoordinates c = chunk.GetCoordinates();
            if (!InSquare(newCenter, c, ld)) toEvict.push_back(c);
        });
    } else {
        // Trailing-edge strips: positions in load(prev) but no longer in load(new).
        const int32_t newXMin = newCenter.X - ld, newXMax = newCenter.X + ld;
        const int32_t newZMin = newCenter.Z - ld, newZMax = newCenter.Z + ld;
        const int32_t prevXMin = prevCenter.X - ld, prevXMax = prevCenter.X + ld;
        const int32_t prevZMin = prevCenter.Z - ld, prevZMax = prevCenter.Z + ld;

        if (prevXMin < newXMin) {
            for (int32_t x = prevXMin; x < newXMin; ++x)
                for (int32_t z = prevZMin; z <= prevZMax; ++z) consider(x, z);
        }
        if (prevXMax > newXMax) {
            for (int32_t x = newXMax + 1; x <= prevXMax; ++x)
                for (int32_t z = prevZMin; z <= prevZMax; ++z) consider(x, z);
        }
        const int32_t overlapXMin = std::max(newXMin, prevXMin);
        const int32_t overlapXMax = std::min(newXMax, prevXMax);
        if (prevZMin < newZMin) {
            for (int32_t z = prevZMin; z < newZMin; ++z)
                for (int32_t x = overlapXMin; x <= overlapXMax; ++x) consider(x, z);
        }
        if (prevZMax > newZMax) {
            for (int32_t z = newZMax + 1; z <= prevZMax; ++z)
                for (int32_t x = overlapXMin; x <= overlapXMax; ++x) consider(x, z);
        }
    }

    if (toEvict.empty()) return false;

    for (const ChunkCoordinates& coords : toEvict) {
        auto chunk = m_Cache.Remove(coords);
        if (!chunk) continue;

        m_Requested.erase(ChunkCache::MakeKey(coords));

        if (chunk->IsDirty) {
            const uint64_t regionID =
                RegionHandler::MakeID(RegionHandler::ChunkToRegion(coords).X,
                                      RegionHandler::ChunkToRegion(coords).Z);
            const uint32_t wi = m_NextWorker % static_cast<uint32_t>(m_Workers.size());
            m_NextWorker = (m_NextWorker + 1) % static_cast<uint32_t>(m_Workers.size());
            {
                std::unique_lock<std::mutex> lock(m_Workers[wi]->mtx);
                m_Workers[wi]->saveQueue.push(SaveTask{std::move(chunk), regionID});
            }
            m_Workers[wi]->cv.notify_one();
        }
    }
    return true;
}

bool ChunkStreamer::InSquare(ChunkCoordinates center, ChunkCoordinates c, int32_t dist) noexcept {
    return std::abs(c.X - center.X) <= dist && std::abs(c.Z - center.Z) <= dist;
}

std::shared_ptr<ChunkStreamer::SharedRegion>
ChunkStreamer::GetOrLoadRegion(uint64_t regionID, ChunkCoordinates regionPos) {
    {
        std::lock_guard<std::mutex> guard(m_SharedRegionsMutex);
        auto it = m_SharedRegions.find(regionID);
        if (it != m_SharedRegions.end()) return it->second;
    }

    // Load outside the map mutex so other workers can still access other regions
    // during disk IO. Another thread may insert the same region concurrently -
    // the duplicate is harmless and dropped by emplace.
    auto sr = std::make_shared<SharedRegion>(regionID, regionPos, m_RegionsDir);
    {
        std::lock_guard<std::mutex> lock(sr->mtx);
        sr->region.Load();
    }

    {
        std::lock_guard<std::mutex> guard(m_SharedRegionsMutex);
        auto [it, inserted] = m_SharedRegions.emplace(regionID, std::move(sr));
        return it->second; // existing entry if another thread won the race
    }
}

// --- Worker thread ---

void ChunkStreamer::WorkerLoop(uint32_t workerIdx) {
    WorkerState& ws = *m_Workers[workerIdx];

    while (m_Running.load()) {
        LoadTask loadTask{};
        SaveTask saveTask{};
        bool hasLoad = false, hasSave = false;

        {
            std::unique_lock<std::mutex> lock(ws.mtx);
            ws.cv.wait(lock, [&] {
                return !m_Running.load()
                    || !ws.loadQueue.empty()
                    || !ws.saveQueue.empty();
            });

            if (!ws.loadQueue.empty()) {
                loadTask = std::move(ws.loadQueue.front());
                ws.loadQueue.pop();
                hasLoad = true;
            } else if (!ws.saveQueue.empty()) {
                saveTask = std::move(ws.saveQueue.front());
                ws.saveQueue.pop();
                hasSave = true;
            }
        }

        if (hasLoad) {
            std::unique_ptr<Chunk> chunk;
            try {
                const ChunkCoordinates regionCoords =
                    RegionHandler::ChunkToRegion(loadTask.coords);
                auto sr = GetOrLoadRegion(loadTask.regionID, regionCoords);
                {
                    std::lock_guard<std::mutex> lock(sr->mtx);
                    if (sr->region.HasChunk(loadTask.coords))
                        chunk = sr->region.ReadChunk(loadTask.coords);
                }
            } catch (const std::exception& e) {
                LOG_ERROR("[ChunkStreamer] Worker %u: exception loading (%d, %d): %s - regenerating",
                          workerIdx, loadTask.coords.X, loadTask.coords.Z, e.what());
                chunk.reset();
            }

            if (!chunk) {
                // Not on disk (or load failed) - generate procedurally.
                chunk = std::make_unique<Chunk>(loadTask.coords.X, loadTask.coords.Z);
                m_Generator.Generate(*chunk);
                chunk->IsDirty = true;
            }

            // Build compact LOD levels off the main thread - the renderer needs
            // them ready before the chunk reaches the main thread cache.
            chunk->GenerateLODs();

            {
                std::unique_lock<std::mutex> lock(ws.mtx);
                ws.completed.push_back(std::move(chunk));
            }
        }

        if (hasSave) {
            const ChunkCoordinates coords       = saveTask.chunk->GetCoordinates();
            const ChunkCoordinates regionCoords = RegionHandler::ChunkToRegion(coords);

            auto sr = GetOrLoadRegion(saveTask.regionID, regionCoords);
            std::lock_guard<std::mutex> lock(sr->mtx);
            if (sr->region.WriteChunk(*saveTask.chunk)) {
                saveTask.chunk->IsDirty = false;
            } else {
                LOG_ERROR("[ChunkStreamer] Worker %u: failed to save chunk (%d, %d)",
                          workerIdx, coords.X, coords.Z);
            }
        }
    }
}
