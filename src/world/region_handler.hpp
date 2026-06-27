#pragma once

#include "world/chunk.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// One entry in head_<id>.bin - 16 bytes, sorted ascending by Key.
struct RegionChunkMeta {
    uint64_t Key;    // ChunkCoordinates::GetKey()  (Z<<32|X)
    uint64_t Offset; // byte offset of this chunk's record in reg_<id>.bin

    RegionChunkMeta() = default;
    RegionChunkMeta(uint64_t key, uint64_t offset) : Key(key), Offset(offset) {}
};
static_assert(sizeof(RegionChunkMeta) == 16);
static_assert(std::is_trivially_copyable_v<RegionChunkMeta>);

// Owns the chunk offset index (head) and the raw blob file (reg) for one region.
// Pure disk I/O - does not hold live Chunk objects in RAM.
class RegionContent {
public:
    RegionContent(std::string headPath, std::string dataPath);

    // Read head_<id>.bin into m_Metas; no-op if file absent (new region).
    void ReadHeader();

    // Write m_Metas to head_<id>.bin and clear the modified flag.
    void SaveHeader();

    // O(log n) lookup. Returns true if chunk exists and sets outOffset.
    bool FindChunk(uint64_t key, uint64_t& outOffset) const;

    // Serialize chunk, append bytes to reg_<id>.bin, upsert meta, flush header.
    // Append-only: existing chunks get a new tail entry; old bytes are orphaned.
    bool WriteChunk(Chunk& chunk);

    // Deserialize chunk stored at the offset recorded for key.
    std::unique_ptr<Chunk> ReadChunk(uint64_t key) const;

    bool IsModified() const noexcept { return m_Modified; }

private:
    // Returns true on exact match (outIndex = position).
    // Returns false on miss (outIndex = lower-bound insertion point).
    bool BinarySearch(uint64_t key, uint64_t& outIndex) const;

    std::string m_HeadPath;
    std::string m_DataPath;
    std::vector<RegionChunkMeta> m_Metas; // sorted by Key
    bool m_Modified = false;
};

// Manages the lifecycle and disk access for a single 32x32 chunk region.
class RegionHandler {
public:
    RegionHandler(uint64_t id, ChunkCoordinates regionPos, const std::string& regDir);
    ~RegionHandler();

    RegionHandler(const RegionHandler&)            = delete;
    RegionHandler& operator=(const RegionHandler&) = delete;

    RegionHandler(RegionHandler&& other) noexcept;
    RegionHandler& operator=(RegionHandler&& other) noexcept;

    bool operator<(uint64_t id) const  { return m_ID < id; }
    bool operator==(uint64_t id) const { return m_ID == id; }
    bool operator>(uint64_t id) const  { return m_ID > id; }

    // Allocate RegionContent and load head file from disk.
    void Load();

    // Flush head to disk and release RegionContent.
    void Unload();

    // True if a record for these chunk coords exists on disk.
    bool HasChunk(ChunkCoordinates coords) const;

    // Serialize chunk and write to disk; update head.
    bool WriteChunk(Chunk& chunk);

    // Read and deserialize chunk from disk. Returns nullptr on miss or error.
    std::unique_ptr<Chunk> ReadChunk(ChunkCoordinates coords) const;

    uint64_t         GetID()        const noexcept { return m_ID; }
    ChunkCoordinates GetRegionPos() const noexcept { return m_RegionPos; }

    // Pack region grid (rx, rz) -> uint64_t using the same Z<<32|X encoding as ChunkCoordinates::GetKey.
    static uint64_t MakeID(int32_t regionX, int32_t regionZ) noexcept;

    // Unpack region ID back to (rx, rz).
    static void DecodeID(uint64_t id, int32_t& outX, int32_t& outZ) noexcept;

    // Convert chunk-grid coords to region-grid coords (floor division, handles negatives).
    static ChunkCoordinates ChunkToRegion(ChunkCoordinates chunk) noexcept;

    // Build the head / data file paths from a region directory and region ID.
    static std::string HeadPath(const std::string& regDir, uint64_t id);
    static std::string DataPath(const std::string& regDir, uint64_t id);

private:
    uint64_t         m_ID;
    ChunkCoordinates m_RegionPos;
    std::string      m_RegDir;
    RegionContent*   m_Context = nullptr;
};
