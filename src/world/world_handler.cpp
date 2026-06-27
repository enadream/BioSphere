#include "world/world_handler.hpp"
#include "io/file_system.hpp"
#include "core/log.hpp"

#include <cmath>
#include <cstring>

// --- Construction ---

WorldHandler::WorldHandler(const ChunkStreamer::Config& cfg)
    : m_WorldDir(cfg.worldDir), m_Streamer(cfg)
{}

// --- Public API ---

void WorldHandler::Init() {
    LoadWorldHeader();
    m_Initialized = true;
    LOG_INFO("[WorldHandler] Initialized. %zu region(s) known.", m_RegionRegistry.size());
}

bool WorldHandler::Update(const glm::vec3& playerPos) {
    if (!m_Initialized) return false;

    // Dead-zone check in the XZ plane only - Y (vertical) movement never changes
    // which chunk column the player occupies.
    const float dx        = playerPos.x - m_LastUpdatePos.x;
    const float dz        = playerPos.z - m_LastUpdatePos.z;
    const float distXZ    = std::sqrt(dx * dx + dz * dz);
    constexpr float threshold = CHUNK_UPDATE_DISTANCE
                          * static_cast<float>(CHUNK_SIZE)
                          * SPHERE_RADIUS;

    if (distXZ < threshold) {
        // Player is inside the dead zone: still drain completed worker results
        // so newly generated chunks reach the cache without waiting for movement.
        return m_Streamer.Tick(m_LastCenter);
    }

    // Player crossed the dead zone - record new anchor and recalculate.
    m_LastUpdatePos = playerPos;
    m_LastCenter    = WorldPosToChunk(playerPos);

    // Register any regions newly overlapped by the load ring.
    EnumerateLoadRingRegions(m_LastCenter);

    return m_Streamer.Tick(m_LastCenter);
}

Chunk* WorldHandler::GetChunk(ChunkCoordinates coords) {
    return m_Streamer.GetChunk(coords);
}

const Chunk* WorldHandler::GetChunk(ChunkCoordinates coords) const {
    return m_Streamer.GetChunk(coords);
}

void WorldHandler::Shutdown() {
    m_Streamer.FlushAll();
    SaveWorldHeader();
    m_Initialized = false;
}

uint32_t WorldHandler::GetRenderDistance() const noexcept {
    return m_Streamer.GetRenderDistance();
}

ChunkCoordinates WorldHandler::GetCenterChunk() const noexcept {
    return m_LastCenter;
}

// --- Private helpers ---

ChunkCoordinates WorldHandler::WorldPosToChunk(const glm::vec3& pos) noexcept {
    // Each chunk spans CHUNK_SIZE * SPHERE_RADIUS world units on each axis.
    constexpr float chunkWorld = static_cast<float>(CHUNK_SIZE) * SPHERE_RADIUS;
    return ChunkCoordinates(
        static_cast<int32_t>(std::floor(pos.x / chunkWorld)),
        static_cast<int32_t>(std::floor(pos.z / chunkWorld))
    );
}

void WorldHandler::EnumerateLoadRingRegions(ChunkCoordinates center) {
    const int32_t ld = static_cast<int32_t>(m_Streamer.GetLoadDistance());

    // Compute the region-grid extent that the load ring overlaps.
    const ChunkCoordinates minRegion =
        RegionHandler::ChunkToRegion(ChunkCoordinates(center.X - ld, center.Z - ld));
    const ChunkCoordinates maxRegion =
        RegionHandler::ChunkToRegion(ChunkCoordinates(center.X + ld, center.Z + ld));

    bool registryChanged = false;
    for (int32_t rz = minRegion.Z; rz <= maxRegion.Z; ++rz)
        for (int32_t rx = minRegion.X; rx <= maxRegion.X; ++rx)
            if (EnsureRegionRegistered(rx, rz)) registryChanged = true;

    // Write the world header at most once per load-ring update, no matter how many
    // new regions were discovered. Stops per-region serial disk writes on fast travel.
    if (registryChanged) SaveWorldHeader();
}

bool WorldHandler::EnsureRegionRegistered(int32_t regionX, int32_t regionZ) {
    const uint64_t key = RegionHandler::MakeID(regionX, regionZ);

    uint64_t idx;
    if (BinarySearch(key, idx)) return false; // already registered

    m_RegionRegistry.insert(m_RegionRegistry.begin() + static_cast<ptrdiff_t>(idx), key);
    return true;
}

bool WorldHandler::LoadWorldHeader() {
    const std::string path = WorldHeaderPath(m_WorldDir);

    std::vector<uint8_t> buffer;
    if (!FileSystem::ReadBinary(path, buffer)) {
        LOG_INFO("[WorldHandler] No world header at %s - starting fresh.", path.c_str());
        return true;
    }

    if (buffer.size() % sizeof(uint64_t) != 0) {
        LOG_ERROR("[WorldHandler] Corrupt world header (size %zu). Ignoring.", buffer.size());
        return false;
    }

    const size_t count = buffer.size() / sizeof(uint64_t);
    m_RegionRegistry.resize(count);
    std::memcpy(m_RegionRegistry.data(), buffer.data(), buffer.size());
    return true;
}

bool WorldHandler::SaveWorldHeader() const {
    const std::string path     = WorldHeaderPath(m_WorldDir);
    const size_t      byteSize = m_RegionRegistry.size() * sizeof(uint64_t);
    if (byteSize == 0) {
        FileSystem::EnsureParentDirectory(path);
        return true;
    }
    return FileSystem::WriteBinary(path,
        reinterpret_cast<const uint8_t*>(m_RegionRegistry.data()), byteSize);
}

bool WorldHandler::BinarySearch(uint64_t targetKey, uint64_t& outIndex) const {
    if (m_RegionRegistry.empty()) {
        outIndex = 0;
        return false;
    }

    int64_t lo = 0;
    int64_t hi = static_cast<int64_t>(m_RegionRegistry.size()) - 1;

    while (lo <= hi) {
        const int64_t mid = lo + (hi - lo) / 2;
        if (m_RegionRegistry[mid] == targetKey) {
            outIndex = static_cast<uint64_t>(mid);
            return true;
        }
        if (m_RegionRegistry[mid] < targetKey)
            lo = mid + 1;
        else
            hi = mid - 1;
    }

    outIndex = static_cast<uint64_t>(lo);
    return false;
}

std::string WorldHandler::WorldHeaderPath(const std::string& worldDir) {
    return worldDir + "/world-header.bin";
}
