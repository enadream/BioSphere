#pragma once

#include "world/chunk.hpp"
#include "world/terrain_generator.hpp"
#include <vector>

class HeightMap { // (CHUNK_SIZE+2)^2 bytes
public:
    HeightMap() = default;

    // the range is [-1, CHUNK_SIZE]
    int16_t GetHeight(int32_t localZ, int32_t localX){
        // mapping [-1, CHUNK_SIZE] -> [0, CHUNK_SIZE+1]
        localX += 1;
        localZ += 1;
        return m_HeightMap[localZ*(CHUNK_SIZE+2) + localX];
    }
    // The range is [-1, CHUNK_SIZE]
    void SetHeight(int32_t localZ, int32_t localX, int16_t val){
        // mapping [-1, CHUNK_SIZE] -> [0, CHUNK_SIZE+1]
        localX += 1;
        localZ+= 1;
        m_HeightMap[localZ*(CHUNK_SIZE+2) + localX] = val;
    }
private:
    std::array<int16_t, (CHUNK_SIZE+2)*(CHUNK_SIZE+2)> m_HeightMap; // 2D int16_t map
};

class ChunkHolder {
public:
    ChunkHolder(uint32_t renderDistance, const Seed256& seed);
    ~ChunkHolder() = default;

    // Main API
    void Update(const glm::vec3& playerPos); // Future: load/unload based on position
    
    // Accessors
    const std::vector<Chunk>& GetChunks() const { return m_Chunks; }
    std::vector<Chunk>& GetChunks() { return m_Chunks; }
    
    uint32_t GetTotalChunkAmount() const { return m_Chunks.size(); }
    uint32_t GetTotalSphereCount() const { return m_CachedSphereCount; }

private:
    void GenerateWorld();
    void GenerateHeightMap(ChunkCoordinates chunkCoord, HeightMap &outHeightMap);
    void GenerateSingleChunk(Chunk& chunk);
    void CalculateAmbientOcclusion(uint32_t chunkId);
    
private:
    std::vector<Chunk> m_Chunks;
    TerrainGenerator m_TerrainGen;
    uint32_t m_RenderDistance; // total amount of chunks player can see
    
    // Cache total sphere count to avoid iterating every frame
    mutable uint32_t m_CachedSphereCount = 0;
};