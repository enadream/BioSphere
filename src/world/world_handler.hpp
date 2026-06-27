#pragma once

#include "world/chunk_streamer.hpp"
#include "glm/glm.hpp"

#include <cstdint>
#include <string>
#include <vector>

// Top-level orchestrator for the infinite world.
//
// Responsibilities:
//   - Owns and persists data/world/world-header.bin (loaded at Init(), kept in RAM,
//     synced to disk whenever a new region is discovered).
//   - Triggers a ChunkStreamer::Tick only when the player has moved at least
//     CHUNK_UPDATE_DISTANCE chunks in the XZ plane from the last trigger point,
//     preventing rapid load/unload when the player oscillates near a chunk boundary.
//   - Provides renderer-facing Chunk* access via GetChunk().
class WorldHandler {
public:
    static constexpr int32_t CHUNK_COUNT = REGION_SIZE * REGION_SIZE;

    explicit WorldHandler(const ChunkStreamer::Config& cfg);
    ~WorldHandler() = default;

    // Read world-header.bin and start the streamer.
    void Init();

    // Call every frame. Triggers streaming update whenever the player moves >=1 chunk.
    // Returns true if the chunk cache changed (new chunks or evictions) - use to gate SyncChunks.
    bool Update(const glm::vec3& playerPos);

    // Read-only chunk access for the renderer (nullptr if not yet loaded).
    Chunk*       GetChunk(ChunkCoordinates coords);
    const Chunk* GetChunk(ChunkCoordinates coords) const;

    // Pass-through to ChunkStreamer; renderer uses this delta to avoid full ring scans.
    std::vector<ChunkCoordinates> ConsumeRecentlyArrived() { return m_Streamer.ConsumeRecentlyArrived(); }

    // Flush all dirty chunks to disk and shut down worker threads.
    void Shutdown();

    uint32_t         GetRenderDistance() const noexcept;
    ChunkCoordinates GetCenterChunk()    const noexcept;

private:
    // Convert world-space position to chunk-grid coordinate.
    static ChunkCoordinates WorldPosToChunk(const glm::vec3& pos) noexcept;

    // Ensure every region overlapping the load ring is in m_RegionRegistry.
    void EnumerateLoadRingRegions(ChunkCoordinates center);

    // Insert region key into m_RegionRegistry if missing. Returns true if a new
    // entry was added (caller is responsible for batched SaveWorldHeader).
    bool EnsureRegionRegistered(int32_t regionX, int32_t regionZ);

    bool LoadWorldHeader();
    bool SaveWorldHeader() const;

    // Binary search m_RegionRegistry.
    // Returns true on exact match (outIndex = position).
    // Returns false on miss (outIndex = lower-bound insertion point).
    bool BinarySearch(uint64_t targetKey, uint64_t& outIndex) const;

    static std::string WorldHeaderPath(const std::string& worldDir);

    std::string           m_WorldDir;
    // world-header.bin in RAM: sorted array of region keys (RegionHandler::MakeID).
    // X/Z are derivable via RegionHandler::DecodeID - no need to store separately.
    std::vector<uint64_t> m_RegionRegistry;

    ChunkStreamer     m_Streamer;

    // Last world position that triggered a load-ring update.
    // Initialized far from origin so the very first Update() always fires.
    // Only XZ components are used for the distance check (Y doesn't affect chunk coords).
    glm::vec3        m_LastUpdatePos{1e9f, 0.0f, 1e9f};

    ChunkCoordinates m_LastCenter{INT32_MAX, INT32_MAX}; // chunk coords at last update
    bool             m_Initialized = false;
};
