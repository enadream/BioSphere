#pragma once

#include "world/chunk_cache.hpp"
#include "world/chunk_generator.hpp"
#include "world/region_handler.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Coordinates chunk availability around the player:
//   - Maintains a load ring (loadDist chunks) and a render ring (renderDist chunks).
//   - Missing chunks are dispatched to worker threads for generation or disk-load.
//   - Completed chunks are flushed into the ChunkCache on each Tick().
//   - Dirty chunks evicted from the cache are saved to disk by a worker thread.
//
// Owns: ChunkCache, active RegionHandlers, worker threads.
// Not thread-safe on the public API - call from main thread only.
class ChunkStreamer {
public:
    struct Config {
        uint32_t    renderDistance = DEFAULT_RENDER_DISTANCE;
        uint32_t    loadDistance   = 0;      // 0 = auto: ceil(renderDistance * LOAD_DISTANCE_FACTOR)
        uint32_t    workerCount    = 2;
        size_t      cacheCapacity  = 0;      // 0 = auto: (loadDist*2+1)^2
        Seed256     seed           = {};
        std::string worldDir       = "data/world";
    };

    explicit ChunkStreamer(const Config& cfg);
    ~ChunkStreamer();

    ChunkStreamer(const ChunkStreamer&) = delete;
    ChunkStreamer& operator=(const ChunkStreamer&) = delete;

    // Called once per frame with the chunk-grid coordinate of the player.
    // Dispatches missing chunks to workers, drains completed results, evicts far chunks.
    // Returns true if the cache contents changed (new chunks or evictions occurred).
    bool Tick(ChunkCoordinates centerChunk);

    // Flush all dirty chunks to disk (blocking) - call before shutdown.
    void FlushAll();

    // Read-only access for the renderer. Returns nullptr if not yet loaded.
    Chunk*       GetChunk(ChunkCoordinates coords);
    const Chunk* GetChunk(ChunkCoordinates coords) const;

    // Take and clear the list of chunks added to the cache since the last call.
    // The renderer uses this delta instead of scanning the full load ring.
    std::vector<ChunkCoordinates> ConsumeRecentlyArrived();

    uint32_t GetRenderDistance() const noexcept { return m_RenderDist; }
    uint32_t GetLoadDistance()   const noexcept { return m_LoadDist; }

    const std::string& GetRegionsDir() const noexcept { return m_RegionsDir; }

private:
    // --- Worker task types ---

    struct LoadTask {
        ChunkCoordinates coords;
        uint64_t         regionID;
    };

    struct SaveTask {
        std::unique_ptr<Chunk> chunk;
        uint64_t               regionID;
    };

    // --- Helpers ---

    // Strip-based ring ops: only iterate the leading edge (RequestLoadRing) /
    // trailing edge (EvictFarChunks) between prevCenter and newCenter, falling
    // back to a full pass when firstTime is true or the rings don't overlap.
    void RequestLoadRing(ChunkCoordinates prevCenter, ChunkCoordinates newCenter, bool firstTime);
    bool EvictFarChunks(ChunkCoordinates prevCenter, ChunkCoordinates newCenter);
    bool DrainCompleted();

    // True if chunkCoords is within squareDist chunks of center (square check, fast).
    static bool InSquare(ChunkCoordinates center, ChunkCoordinates c, int32_t dist) noexcept;

    // Worker thread entry point.
    void WorkerLoop(uint32_t workerIdx);

    // --- Per-worker state ---

    struct WorkerState {
        std::queue<LoadTask>            loadQueue;
        std::queue<SaveTask>            saveQueue;
        std::vector<std::unique_ptr<Chunk>> completed; // ready to move to cache

        std::mutex              mtx;
        std::condition_variable cv;
    };

    // --- Members ---

    uint32_t    m_RenderDist;
    uint32_t    m_LoadDist;
    std::string m_WorldDir;
    std::string m_RegionsDir;

    // Shared region handlers: each region is loaded exactly once, kept in memory,
    // and accessed by every worker via the per-region mutex. Headers are written
    // only on FlushAll / shutdown - WriteChunk just appends data and updates the
    // in-memory index. Workers touching different regions parallelise fully.
    struct SharedRegion {
        RegionHandler region;
        std::mutex    mtx;

        SharedRegion(uint64_t id, ChunkCoordinates pos, const std::string& dir)
            : region(id, pos, dir) {}
    };

    std::mutex                                                    m_SharedRegionsMutex;
    std::unordered_map<uint64_t, std::shared_ptr<SharedRegion>>   m_SharedRegions;

    std::shared_ptr<SharedRegion> GetOrLoadRegion(uint64_t regionID, ChunkCoordinates regionPos);

    ChunkCache     m_Cache;
    ChunkGenerator m_Generator;

    // Per-worker state (stable addresses via unique_ptr).
    std::vector<std::unique_ptr<WorkerState>> m_Workers;
    std::vector<std::thread>                  m_Threads;
    std::atomic<bool>                         m_Running{true};
    uint32_t                                  m_NextWorker = 0;

    // Keys currently in the cache or en-route to it (avoids double-dispatch).
    std::unordered_set<uint64_t> m_Requested;

    // Chunks added to the cache since the last ConsumeRecentlyArrived() call.
    std::vector<ChunkCoordinates> m_RecentlyArrived;

    // Last center passed to Tick - guards the expensive ring/eviction operations.
    ChunkCoordinates m_LastTickCenter{0, 0};
    bool             m_TickCenterValid = false;
};
