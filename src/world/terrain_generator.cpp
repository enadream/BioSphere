#include "world/terrain_generator.hpp"
#include <algorithm>
#include <bit>

TerrainGenerator::TerrainGenerator(const Seed256& seed) : m_MasterSeed(seed) {
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
    return static_cast<int>(std::bit_cast<int32_t>(hash));
}

void TerrainGenerator::InitNoise() {
    // Frequencies are divided by FEATURE_SCALE so features widen proportionally with height.
    // Continental - 20 km scale geography (ocean vs plains vs mountain regions)
    m_BaseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_BaseNoise.SetSeed(DeriveSeed("continental"));
    m_BaseNoise.SetFrequency(0.00005f / FEATURE_SCALE);
    m_BaseNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_BaseNoise.SetFractalOctaves(3);
    m_BaseNoise.SetFractalLacunarity(2.0f);
    m_BaseNoise.SetFractalGain(0.5f);

    // Peaks & Valleys - 5 km scale ridged FBm; 5 octaves add sub-ridge detail down to ~300 m
    m_MountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_MountainNoise.SetSeed(DeriveSeed("peaks_valleys"));
    m_MountainNoise.SetFrequency(0.0002f / FEATURE_SCALE);
    m_MountainNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    m_MountainNoise.SetFractalOctaves(5);
    m_MountainNoise.SetFractalLacunarity(2.0f);
    m_MountainNoise.SetFractalGain(0.5f);

    // Erosion - 7 km scale; controls how smooth vs jagged each region is
    m_TerrainMask.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_TerrainMask.SetSeed(DeriveSeed("erosion"));
    m_TerrainMask.SetFrequency(0.00015f / FEATURE_SCALE);
    m_TerrainMask.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_TerrainMask.SetFractalOctaves(3);
    m_TerrainMask.SetFractalLacunarity(2.0f);
    m_TerrainMask.SetFractalGain(0.5f);

    // Domain warp - 4 km scale distortion; makes ridge lines wind naturally
    m_TreeDensityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_TreeDensityNoise.SetSeed(DeriveSeed("domain_warp"));
    m_TreeDensityNoise.SetFrequency(0.00025f / FEATURE_SCALE);

    // Detail - 67 m scale surface roughness (boulders / rocky ground texture)
    m_DetailNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_DetailNoise.SetSeed(DeriveSeed("terrain_detail"));
    m_DetailNoise.SetFrequency(0.015f / FEATURE_SCALE);
    m_DetailNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_DetailNoise.SetFractalOctaves(3);
    m_DetailNoise.SetFractalLacunarity(2.0f);
    m_DetailNoise.SetFractalGain(0.5f);

    // Biome / Rock (reserved for future ecology, not used in height calculation)
    m_BiomeNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_BiomeNoise.SetSeed(DeriveSeed("biome"));
    m_BiomeNoise.SetFrequency(0.001f);

    m_RockNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    m_RockNoise.SetSeed(DeriveSeed("rocks"));
    m_RockNoise.SetFrequency(0.1f);
}

float TerrainGenerator::GetHeight(float x, float z) const {
    // Linear remap from [a,b] to [c,d]
    auto remap = [](float v, float a, float b, float c, float d) noexcept {
        return c + (v - a) / (b - a) * (d - c);
    };
    auto smoothStep = [](float e0, float e1, float v) noexcept {
        v = std::clamp((v - e0) / (e1 - e0), 0.0f, 1.0f);
        return v * v * (3.0f - 2.0f * v);
    };

    // --- 1. Domain warp ---
    // Two independent displacements from one noise via large prime offset.
    // Warping breaks the uniform grid look and creates winding ridges and valleys.
    const float warpX = m_TreeDensityNoise.GetNoise(x,             z)             * m_Params.WarpStrength;
    const float warpZ = m_TreeDensityNoise.GetNoise(x + 31743.0f,  z + 31743.0f)  * m_Params.WarpStrength;

    // Soft warp for continents (gently curved coastlines).
    // Full warp for peaks (strongly winding ridge lines).
    const float cxSoft = x + warpX * 0.25f,  czSoft = z + warpZ * 0.25f;
    const float cxFull = x + warpX,           czFull = z + warpZ;

    // --- 2. Continentalness [-1, 1] --- 
    // Large scale: determines whether a region is ocean, plains, or highland.
    const float continental = m_BaseNoise.GetNoise(cxSoft, czSoft);

    // --- 3. Erosion [0, 1] --- 
    // 0 = rough / uneroded sharp ridges.
    // 1 = smooth / heavily eroded flat plains.
    const float erosion = smoothStep(-0.4f, 0.5f, m_TerrainMask.GetNoise(x, z));

    // --- 4. Peaks & Valleys [0, 1] ---
    // Ridged FBm: 1 = mountain ridge crest,  0 = valley floor.
    const float pv = m_MountainNoise.GetNoise(cxFull, czFull);

    // --- 5. Continentalness -> base elevation (piecewise spline) ---
    // Each segment maps a continental range to a distinct terrain zone.
    float baseH;
    const float c = continental;
    if      (c < -0.45f) baseH = remap(c, -1.0f, -0.45f, m_Params.OceanFloor,          m_Params.ShoreLevel - 8.0f);
    else if (c <  0.0f ) baseH = remap(c, -0.45f,  0.0f,  m_Params.ShoreLevel - 8.0f,  m_Params.ShoreLevel);
    else if (c <  0.35f) baseH = remap(c,  0.0f,   0.35f, m_Params.ShoreLevel,          m_Params.PlainLevel);
    else if (c <  0.65f) baseH = remap(c,  0.35f,  0.65f, m_Params.PlainLevel,          m_Params.HighlandLevel);
    else                 baseH = remap(c,  0.65f,  1.0f,  m_Params.HighlandLevel,       m_Params.HighlandLevel * 1.5f);

    // --- 6. Mountain peaks --- 
    // Peaks only rise in highland continental regions AND where erosion is low.
    const float mountainInfluence = smoothStep(0.25f, 0.65f, c) * (1.0f - erosion);
    const float peak              = pv * m_Params.PeakHeight * mountainInfluence;

    // --- 7. Surface detail --- 
    // Suppressed in smooth eroded zones, stronger on rough highland terrain.
    const float detail = m_DetailNoise.GetNoise(x, z)
                       * m_Params.DetailStrength * (0.2f + 0.8f * (1.0f - erosion));

    return baseH + peak + detail;
}