#include "chunk.hpp"


ChunkHolder::ChunkHolder(uint32_t chunk_am, float sphere_radius) : chunkAmount(chunk_am), sphereRadius(sphere_radius) {
    totalNumOfSpheres = 0;
    generateHeightMap();
    generateChunks();
}

void ChunkHolder::generateChunks() {
    uint32_t z_offset = 1;
    uint32_t x_offset = 1;

    chunks.reserve(chunkAmount * chunkAmount);
    for (uint32_t i = 0; i < chunkAmount; i++){
        // process chunks row by row
        for (uint32_t j = 0; j < chunkAmount; j++){
            chunks.emplace_back();
            // generate last appended chunk
            generateChunk(z_offset, x_offset, chunks.size()-1);
            // increase x offset
            x_offset += CHUNK_SIZE;
        }
        x_offset = 1;
        z_offset += CHUNK_SIZE;
        
    }
}

void ChunkHolder::generateChunk(const uint32_t z_off, const uint32_t x_off, uint32_t chunk_id) {
    chunks[chunk_id].m_StartIndex = totalNumOfSpheres;
    
    // set bounding box, because these are origins of spheres I need to add
    chunks[chunk_id].m_BoundBox.minZ = z_off*sphereRadius - sphereRadius;
    chunks[chunk_id].m_BoundBox.minZ = (z_off+CHUNK_SIZE-1)*sphereRadius + sphereRadius;
    chunks[chunk_id].m_BoundBox.minX = x_off*sphereRadius - sphereRadius;
    chunks[chunk_id].m_BoundBox.maxX = (x_off+CHUNK_SIZE-1)*sphereRadius + sphereRadius;
    // set the y value to the first value
    chunks[chunk_id].m_BoundBox.minY = heightMap[z_off][x_off] - sphereRadius;
    chunks[chunk_id].m_BoundBox.maxY = heightMap[z_off][x_off] + sphereRadius;


    for (uint32_t i = z_off; i < z_off+CHUNK_SIZE; i++){
        float zValue = i*sphereRadius;
        for (uint32_t j = x_off; j < x_off+CHUNK_SIZE; j++){
            float xValue = j*sphereRadius;
            float yValue = heightMap[i][j];

            chunks[chunk_id].m_Positions.emplace_back(xValue, yValue, zValue);
            chunks[chunk_id].m_BoundBox.maxY = glm::max(yValue+sphereRadius, chunks[chunk_id].m_BoundBox.maxY);
            chunks[chunk_id].m_BoundBox.minY = glm::min(yValue-sphereRadius, chunks[chunk_id].m_BoundBox.minY);

            // check 8 neighbour find the highest difference , [z][x]
            constexpr int8_t neighbs[8][2] = {{-1, 0}, {0, -1}, {0, 1}, {1, 0}, // up, left, right, down
                {-1, -1}, {-1, 1}, {1, -1}, {1, 1}}; 
            float maxDiff = 0.0f;
            for (int k = 0; k < 8; k++){
                float difference = yValue - heightMap[i+neighbs[k][0]][j+neighbs[k][1]];
                if (difference > maxDiff)
                    maxDiff = difference;
            }

            // if difference bigger than sphereRadius create more spheres to fill the gap
            int64_t fillAmount = (maxDiff)/(sphereRadius*2);
            for (uint32_t f = 1; f <= fillAmount; f++){
                float fillY = yValue - f*(sphereRadius*2);
                chunks[chunk_id].m_Positions.emplace_back(xValue, fillY, zValue);
            }

            // check minimum y value again
            float fillY = yValue - fillAmount*(sphereRadius*2);
            chunks[chunk_id].m_BoundBox.minY = glm::min(fillY-sphereRadius, chunks[chunk_id].m_BoundBox.minY);
        }
    }
    // add total number of chunks
    totalNumOfSpheres += chunks[chunk_id].m_Positions.size();
}

void ChunkHolder::generateHeightMap() {
    // init noise
    initNoise();

    const uint32_t terrainX = chunkAmount * CHUNK_SIZE + 2; 
    const uint32_t terrainZ = chunkAmount * CHUNK_SIZE + 2;

    heightMap.reserve(terrainZ);

    // find terrain discrete hight map
    for (uint32_t i = 0; i < terrainZ; i++){
        heightMap.emplace_back();
        heightMap[i].reserve(terrainX);
        float zValue = i*sphereRadius;

        for (uint32_t j = 0; j < terrainX; j++){
            float xValue = j*sphereRadius;
            float yOffset = ((i+j) % 2) * sphereRadius;
            float yValue = getTerrainHeight(xValue, zValue); //getTerrainHeight(xValue, zValue) - yOffset;
            int64_t discrete = (yValue/(sphereRadius*2));
            yValue = (discrete * (sphereRadius*2)) + yOffset;

            heightMap[i].push_back(yValue);
        }
    }
}

float ChunkHolder::getTerrainHeight(float x, float z){
    auto smoothStep = [](float edge0, float edge1, float x){
        x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return x * x * (3.0f - 2.0f * x);
    };

    // Calculate base terrain (normalized to 0-1 range)
    float base = (base_noise.GetNoise(x, z) + 1.0f) * 0.5f;
    
    // Add medium-scale detail
    float detail = detail_noise.GetNoise(x, z) * 0.15f;
    
    // Calculate mountain pattern
    float mountains = (1.0f - std::abs(mountain_noise.GetNoise(x, z))) * 2.0f;
    
    // Calculate terrain mask for biome distribution
    float mask = smoothStep(0.3f, 0.6f, (terrain_mask.GetNoise(x, z) + 1.0f) * 0.5f);
    
    // Combine elements
    float height = base * 5.0f;                 // Base elevation scale
    height += detail * 25.0f;                     // Medium detail scale
    height += pow(base, 4.0f) * 200.0f * mask;    // Mountainous areas
    height += mountains * 50.0f * mask;           // Add sharp mountain features
    
    // Apply water level and return   
    //return std::max(height - 50.0f, 0.0f);  // Subtract 50 to create water regions
    return height - 50.0f;
}

void ChunkHolder::initNoise(){
    if (!noise_initialized) {
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
        detail_noise.SetFrequency(0.01f); // 0.1f default
        detail_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        detail_noise.SetFractalOctaves(3);

        // Configure mountain noise (sharp ridges)
        mountain_noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
        mountain_noise.SetSeed(6969);
        mountain_noise.SetFrequency(0.0001f); // default 0.001f
        mountain_noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Hybrid);
        mountain_noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Div);

        // Configure terrain mask (biome distribution)
        terrain_mask.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        terrain_mask.SetSeed(911);
        terrain_mask.SetFrequency(0.0005f);
        terrain_mask.SetFractalType(FastNoiseLite::FractalType_Ridged);
        terrain_mask.SetFractalOctaves(3);

        noise_initialized = true;
    }
}