#include "world/terrain_generator.hpp"
#include <algorithm>
#include <cmath>

TerrainGenerator::TerrainGenerator(const Seed256 seed) : m_MasterSeed(seed) {
    InitNoise();
}

int TerrainGenerator::DeriveSeed(std::string_view featureID) const {
    // FNV-1a Hash Implementation
    // We hash the 32 bytes of the seed + the bytes of the feature string.
    
    uint32_t hash = 2166136261u; // FNV offset basis
    const uint32_t prime = 16777619u; // FNV prime

    // 1. Hash the Master Seed Data
    const uint8_t* seedBytes = reinterpret_cast<const uint8_t*>(m_MasterSeed.Data.data());
    for (size_t i = 0; i < sizeof(m_MasterSeed.Data); ++i) {
        hash ^= seedBytes[i];
        hash *= prime;
    }

    // 2. Hash the Feature Identifier (e.g., "trees", "mountains")
    for (char c : featureID) {
        hash ^= static_cast<uint8_t>(c);
        hash *= prime;
    }

    // The result is a unique, deterministic 32-bit seed for this feature
    return static_cast<int>(hash);
}

void TerrainGenerator::InitNoise() {
    // --- 1. Base Terrain Shape ---
    // Uses seed derived from "base_terrain"
    m_BaseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_BaseNoise.SetSeed(DeriveSeed("base_terrain"));
    m_BaseNoise.SetFrequency(0.0025f);
    m_BaseNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_BaseNoise.SetFractalOctaves(4);
    m_BaseNoise.SetFractalLacunarity(2.0f);
    m_BaseNoise.SetFractalGain(0.5f);

    // --- 2. Detail Noise ---
    m_DetailNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_DetailNoise.SetSeed(DeriveSeed("terrain_detail"));
    m_DetailNoise.SetFrequency(0.01f);
    m_DetailNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_DetailNoise.SetFractalOctaves(3);

    // --- 3. Mountains ---
    // Uses Cellular noise for sharp ridges
    m_MountainNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    m_MountainNoise.SetSeed(DeriveSeed("mountains"));
    m_MountainNoise.SetFrequency(0.0001f);
    m_MountainNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Hybrid);
    m_MountainNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Div);

    // --- 4. Biome Distribution / Mask ---
    m_TerrainMask.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_TerrainMask.SetSeed(DeriveSeed("biome_mask"));
    m_TerrainMask.SetFrequency(0.0005f);
    m_TerrainMask.SetFractalType(FastNoiseLite::FractalType_Ridged);
    m_TerrainMask.SetFractalOctaves(3);

    // --- 5. Vegetation (Trees) ---
    m_TreeDensityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_TreeDensityNoise.SetSeed(DeriveSeed("vegetation_trees"));
    m_TreeDensityNoise.SetFrequency(0.05f); // High frequency for individual tree placement logic

    // --- 6. Biomes (Temp/Moisture) ---
    m_BiomeNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_BiomeNoise.SetSeed(DeriveSeed("biome_properties"));
    m_BiomeNoise.SetFrequency(0.001f);

    // --- 7. Rocks/Decorations ---
    m_RockNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    m_RockNoise.SetSeed(DeriveSeed("rocks"));
    m_RockNoise.SetFrequency(0.1f);
}

float TerrainGenerator::GetHeight(float x, float z) const {
    auto smoothStep = [](float edge0, float edge1, float val){
        val = std::clamp((val - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return val * val * (3.0f - 2.0f * val);
    };

    // Calculate base terrain (normalized to 0-1 range)
    float base = (m_BaseNoise.GetNoise(x, z) + 1.0f) * 0.5f;
    
    // Add medium-scale detail
    float detail = m_DetailNoise.GetNoise(x, z) * 0.15f;
    
    // Calculate mountain pattern
    float mountains = (1.0f - std::abs(m_MountainNoise.GetNoise(x, z))) * 2.0f;
    
    // Calculate terrain mask for biome distribution
    float mask = smoothStep(0.3f, 0.6f, (m_TerrainMask.GetNoise(x, z) + 1.0f) * 0.5f);
    
    // Combine elements
    float height = base * 5.0f;                 
    height += detail * 25.0f;                     
    height += std::pow(base, 4.0f) * 200.0f * mask;    
    height += mountains * 50.0f * mask;           
    
    return height - 50.0f;
}