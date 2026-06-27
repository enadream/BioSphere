#pragma once

#include "world/chunk.hpp"
#include "world/terrain_generator.hpp"

class HeightMap { // (CHUNK_SIZE+2)^2 bytes
public:
    HeightMap() = default;

    // the range is [-1, CHUNK_SIZE]
    int16_t GetHeight(int32_t localZ, int32_t localX) const {
        // mapping [-1, CHUNK_SIZE] -> [0, CHUNK_SIZE+1]
        localX += 1;
        localZ += 1;
        return m_HeightMap[localZ*(CHUNK_SIZE+2) + localX];
    }
    // The range is [-1, CHUNK_SIZE]
    void SetHeight(int32_t localZ, int32_t localX, int16_t val){
        // mapping [-1, CHUNK_SIZE] -> [0, CHUNK_SIZE+1]
        localX += 1;
        localZ += 1;
        m_HeightMap[localZ*(CHUNK_SIZE+2) + localX] = val;
    }
private:
    std::array<int16_t, (CHUNK_SIZE+2)*(CHUNK_SIZE+2)> m_HeightMap; // 2D int16_t map
};

// Generates the content of a single Chunk from terrain noise.
// Stateless -- depends only on TerrainGenerator and chunk coordinates.
// Single Responsibility: knows how to fill a Chunk, nothing else.
class ChunkGenerator {
public:
    explicit ChunkGenerator(const Seed256& seed);

    // Fills chunk with spheres derived from noise. Chunk must already have coordinates set.
    void Generate(Chunk& chunk) const;

private:
    void GenerateHeightMap(const ChunkCoordinates& coords, HeightMap& outMap) const ;
    void InitBounds(Chunk& chunk, const HeightMap& heightMap) const;
    void FillChunk(Chunk& chunk, const HeightMap& heightMap) const ;

    TerrainGenerator m_TerrainGen;
};