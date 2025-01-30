#include <glm/glm.hpp>
#include <algorithm>
#include "math/FastNoiseLite.hpp"

// Smoothstep function for natural transitions
float smoothstep(float edge0, float edge1, float x) {
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

float getTerrainHeight(float x, float z) {
    // Initialize noise generators (static to maintain state between calls)
    static FastNoiseLite base_noise;
    static FastNoiseLite detail_noise;
    static FastNoiseLite mountain_noise;
    static FastNoiseLite terrain_mask;
    static bool initialized = false;

    if (!initialized) {
        // Configure base terrain noise (rolling hills)
        base_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        base_noise.SetSeed(98748646);
        base_noise.SetFrequency(0.0025f); // default 0.0025f
        base_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        base_noise.SetFractalOctaves(4);
        base_noise.SetFractalLacunarity(2.0f);
        base_noise.SetFractalGain(0.5f);

        // Configure detail noise (surface imperfections)
        detail_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        detail_noise.SetSeed(4242);
        detail_noise.SetFrequency(0.05f); // 0.1f
        detail_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        detail_noise.SetFractalOctaves(3);

        // Configure mountain noise (sharp ridges)
        mountain_noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
        mountain_noise.SetSeed(6969);
        mountain_noise.SetFrequency(0.001f);
        mountain_noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Hybrid);
        mountain_noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Div);

        // Configure terrain mask (biome distribution)
        terrain_mask.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        terrain_mask.SetSeed(911);
        terrain_mask.SetFrequency(0.0005f);
        terrain_mask.SetFractalType(FastNoiseLite::FractalType_Ridged);
        terrain_mask.SetFractalOctaves(3);

        initialized = true;
    }

    // Calculate base terrain (normalized to 0-1 range)
    float base = (base_noise.GetNoise(x, z) + 1.0f) * 0.5f;
    
    // Add medium-scale detail
    float detail = detail_noise.GetNoise(x, z) * 0.15f;
    
    // Calculate mountain pattern
    float mountains = (1.0f - std::abs(mountain_noise.GetNoise(x, z))) * 2.0f;
    
    // Calculate terrain mask for biome distribution
    float mask = smoothstep(0.3f, 0.6f, (terrain_mask.GetNoise(x, z) + 1.0f) * 0.5f);
    
    // Combine elements
    float height = base * 5.0f;                 // Base elevation scale
    height += detail * 25.0f;                     // Medium detail scale
    height += pow(base, 4.0f) * 200.0f * mask;    // Mountainous areas
    height += mountains * 50.0f * mask;           // Add sharp mountain features
    
    // Apply water level and return   
    //return std::max(height - 50.0f, 0.0f);  // Subtract 50 to create water regions
    //int discrete = (height + yOffset - 50.0f) / 0.5;  // Subtract 50 to create water regions
    //return (discrete * 0.5) - yOffset;
    return height - 50.0f;
}