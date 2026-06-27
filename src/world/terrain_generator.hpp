#pragma once

#include "util/math/FastNoiseLite.hpp"
#include "world/config.hpp"
#include <cstdint>
#include <array>
#include <string_view>

struct TerrainParams {
    // Elevation key-points in world units; 1 world unit = 1 m (sphere diameter)
    // All heights are multiplied by FEATURE_SCALE so the landscape scales proportionally.
    float OceanFloor     = -400.0f  * FEATURE_SCALE;  // deepest ocean bed
    float ShoreLevel     =  -20.0f  * FEATURE_SCALE;  // waterline / beach
    float PlainLevel     =   80.0f  * FEATURE_SCALE;  // top of lowland plains
    float HighlandLevel  =  350.0f  * FEATURE_SCALE;  // highland / foothills base
    float PeakHeight     = 1200.0f  * FEATURE_SCALE;  // max rise of mountain peaks above highland base
    // Noise strengths
    float DetailStrength =    5.0f  * FEATURE_SCALE;  // micro surface roughness amplitude
    float WarpStrength   =  500.0f  * FEATURE_SCALE;  // domain-warp displacement in world units
};

// 256-bit Seed Structure (4 x 64-bit integers)
struct Seed256 {
    std::array<uint64_t, 4> Data;

    Seed256() : Data{0, 0, 0, 0} {}
    Seed256(uint64_t s1, uint64_t s2, uint64_t s3, uint64_t s4) 
        : Data{s1, s2, s3, s4} {}
        
    // Example helper to generate a random seed (implementation depends on your RNG)
    // static Seed256 Random(); 
};

class TerrainGenerator {
public:
    // Accepts the full 256-bit seed
    TerrainGenerator(const Seed256& seed);

    // Returns the terrain height at global world coordinates (x, z)
    float GetHeight(float x, float z) const;

    // Future: GetBiome(x, z), GetTreeDensity(x, z), etc.

private:
    void InitNoise();
    
    // Hashes the 256-bit master seed + a string identifier to produce a unique 32-bit seed
    int DeriveSeed(std::string_view featureID) const;

private:
    Seed256 m_MasterSeed;
    TerrainParams m_Params;

    // --- Noise Modules ---
    // Terrain Shape
    FastNoiseLite m_BaseNoise;
    FastNoiseLite m_DetailNoise;
    FastNoiseLite m_MountainNoise;
    FastNoiseLite m_TerrainMask;

    // Ecology (New additions based on your request)
    FastNoiseLite m_TreeDensityNoise;
    FastNoiseLite m_BiomeNoise;
    FastNoiseLite m_RockNoise;
};