#include "chunk.hpp"

Chunk::Chunk(const int32_t x, const int32_t z) : startPosition(x, z), bufferVertexOffset(0){}

ChunkHolder::ChunkHolder(uint32_t chunk_am, float sphere_radius) : chunkAmount(chunk_am), sphereRadius(sphere_radius), totalNumOfSpheres(0) {
    // init noise
    initNoise();
    generateChunks();
}

void ChunkHolder::generateChunks() {
    // start position of the chunk
    ChunkPos chunkPos(0, 0), startOffset(0, 0);

    chunks.reserve(chunkAmount * chunkAmount);
    for (uint32_t i = 0; i < chunkAmount; i++){
        // process chunks row by row
        for (uint32_t j = 0; j < chunkAmount; j++){
            // set the chunk value
            chunkPos.z = i*CHUNK_SIZE + startOffset.z;
            chunkPos.x = j*CHUNK_SIZE + startOffset.x;
            // generate height map for chunk
            generateHeightMapForChunk(chunkPos);
            // create a chunk
            chunks.emplace_back(chunkPos.x, chunkPos.z);
            // fill the last chunk
            generateChunk(chunks.size()-1);
        }
    }
}

void ChunkHolder::generateChunk(uint32_t chunk_id) {
    // set bounding box, because these are origins of spheres I need to add and subtract the radius to find bb in x and z axes
    chunks[chunk_id].boundBox.m_Min.z = chunks[chunk_id].startPosition.z*sphereRadius - sphereRadius;
    chunks[chunk_id].boundBox.m_Max.z = (chunks[chunk_id].startPosition.z+CHUNK_SIZE-1)*sphereRadius + sphereRadius;
    chunks[chunk_id].boundBox.m_Min.x = chunks[chunk_id].startPosition.x*sphereRadius - sphereRadius;
    chunks[chunk_id].boundBox.m_Max.x = (chunks[chunk_id].startPosition.x+CHUNK_SIZE-1)*sphereRadius + sphereRadius;
    // set the y value to the first value, this value can be updated with each sphere
    chunks[chunk_id].boundBox.m_Min.y = chunkHeightMap.getHeight(0,0)*sphereRadius - sphereRadius;
    chunks[chunk_id].boundBox.m_Max.y = chunkHeightMap.getHeight(0,0)*sphereRadius + sphereRadius;

    for (int32_t z = 0; z < CHUNK_SIZE; z++){ // iterate over z axis
        float zValue = sphereRadius * (z + chunks[chunk_id].startPosition.z);
        for (int32_t x = 0; x < CHUNK_SIZE; x++){ // iterate over x axis
            constexpr int8_t neighbs[8][2] = {{-1, 0}, {0, -1}, {0, 1}, {1, 0}, // up, left, right, down
                {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

            float xValue = sphereRadius * (x + chunks[chunk_id].startPosition.x);
            int16_t discreteY = chunkHeightMap.getHeight(z, x);
            float yValue = sphereRadius * discreteY;
            
            if ((z+x)%2 == 1){ // od layer
                // check if 4 neighbour if they have same hight value then you don't need to append this sphere
                int16_t hightLevel = chunkHeightMap.getHeight(z+neighbs[0][0], x+neighbs[0][1]);
                bool sameHight = true;

                for (uint32_t k = 1; k < 4; k++){
                    if (hightLevel != chunkHeightMap.getHeight(z+neighbs[k][0], x+neighbs[k][1])){
                        sameHight = false;
                        break;
                    }
                }
                if (sameHight)
                    continue;
            }
            
            chunks[chunk_id].spheres.emplace_back(xValue, yValue, zValue, sphereRadius);
            chunks[chunk_id].boundBox.m_Max.y = glm::max(yValue + sphereRadius, chunks[chunk_id].boundBox.m_Max.y);
            chunks[chunk_id].boundBox.m_Min.y = glm::min(yValue - sphereRadius, chunks[chunk_id].boundBox.m_Min.y);

            // check 8 neighbour find the highest difference , [z][x]
            int32_t maxDiff = 0;
            for (int16_t k = 0; k < 8; k++){
                int32_t difference = discreteY - chunkHeightMap.getHeight(z+neighbs[k][0], x+neighbs[k][1]);
                if (difference > maxDiff)
                    maxDiff = difference;
            }

            // if difference bigger than sphereRadius create more spheres to fill the gap
            for (int16_t f = 2; f <= maxDiff; f += 2){
                int32_t fillY = discreteY - f;
                chunks[chunk_id].spheres.emplace_back(xValue, fillY*sphereRadius, zValue, sphereRadius);
            }

            // check minimum y value again
            int16_t maxDiscDiff = discreteY - (maxDiff/2)*2; // if max diff has to be multiple of 2
            chunks[chunk_id].boundBox.m_Min.y = glm::min(maxDiscDiff*sphereRadius - sphereRadius, chunks[chunk_id].boundBox.m_Min.y);
        }
    }    
    // add total number of chunks
    totalNumOfSpheres += chunks[chunk_id].spheres.size();
}

void ChunkHolder::generateHeightMapForChunk(const ChunkPos chunk_pos) {
    // generate [-1+x, x+1] height map

    // find terrain discrete hight map
    for (int32_t z = -1; z < CHUNK_SIZE+1; z++){
        float zValue = (z+chunk_pos.z)*sphereRadius;

        for (int32_t x = -1; x < CHUNK_SIZE+1; x++){
            float xValue = (x+chunk_pos.x)*sphereRadius;
            float heightValue = getTerrainHeight(xValue, zValue); //getTerrainHeight(xValue, zValue) - yOffset;
            int16_t discreteY = heightValue/sphereRadius;

            //int16_t layerOffset = (i+j) % 2;
            //int16_t heightOffset = discreteY % 2;
            discreteY -= ((z+x) + discreteY) % 2;
            // store the data to the chunk map
            chunkHeightMap.setHeight(z, x, discreteY);
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


// for (uint32_t i = z_off; i < z_off+CHUNK_SIZE; i++){
//     uint8_t zValue = i-z_off;
//     for (uint32_t j = x_off; j < x_off+CHUNK_SIZE; j++){
//         constexpr int8_t neighbs[8][2] = {{-1, 0}, {0, -1}, {0, 1}, {1, 0}, // up, left, right, down
//             {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

//         uint8_t xValue = j-x_off;
//         int16_t yValue = heightMap[i][j];

//         if ((i+j)%2 == 1){
//             continue;
            
//             // check if 4 neighbour if they have same hight value then you don't need to append this sphere
//             int16_t hightLevel = heightMap[i+neighbs[0][0]][j+neighbs[0][1]];
//             bool sameHight = true;

//             for (uint32_t k = 1; k < 4; k++){
//                 if (hightLevel != heightMap[i+neighbs[k][0]][j+neighbs[k][1]]){
//                     sameHight = false;
//                     break;
//                 }
//             }
//             if (sameHight)
//                 chunks[chunk_id].m_Spheres.emplace_back(xValue, yValue, zValue);
//             continue;
//         }
        
//         chunks[chunk_id].m_Spheres.emplace_back(xValue, yValue, zValue);
//         chunks[chunk_id].m_ChunkInfo.boundBox.m_Max.y = glm::max(yValue*sphereRadius + sphereRadius, chunks[chunk_id].m_ChunkInfo.boundBox.m_Max.y);
//         chunks[chunk_id].m_ChunkInfo.boundBox.m_Min.y = glm::min(yValue*sphereRadius - sphereRadius, chunks[chunk_id].m_ChunkInfo.boundBox.m_Min.y);

//         // check 8 neighbour find the highest difference , [z][x]
//         int16_t maxDiff = 0;
//         for (int16_t k = 0; k < 8; k++){
//             int16_t difference = yValue - heightMap[i+neighbs[k][0]][j+neighbs[k][1]];
//             if (difference > maxDiff)
//                 maxDiff = difference;
//         }

//         // if difference bigger than sphereRadius create more spheres to fill the gap
//         for (int16_t f = 2; f <= maxDiff; f += 2){
//             int16_t fillY = yValue - f;
//             chunks[chunk_id].m_Spheres.emplace_back(xValue, fillY, zValue);
//         }

//         // check minimum y value again
//         int16_t fillY = yValue - (maxDiff/2)*2;
//         chunks[chunk_id].m_ChunkInfo.boundBox.m_Min.y = glm::min(fillY*sphereRadius - sphereRadius, chunks[chunk_id].m_ChunkInfo.boundBox.m_Min.y);
//     }
// }


// void ChunkHolder::generateSphere(const int32_t x_pos, const int32_t z_pos) {
//     constexpr int8_t neighbs[8][2] = {{-1, 0}, {0, -1}, {0, 1}, {1, 0}, // up, left, right, down
//         {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};


//     int16_t yValue = heightMap[z_pos][x_pos];

//     if ((x_pos+z_pos)%2 == 1){
//         return;
        
//         // check if 4 neighbour if they have same hight value then you don't need to append this sphere
//         int16_t hightLevel = heightMap[z_pos+neighbs[0][0]][x_pos+neighbs[0][1]];
//         bool sameHight = true;

//         for (uint32_t k = 1; k < 4; k++){
//             if (hightLevel != heightMap[z_pos+neighbs[k][0]][x_pos+neighbs[k][1]]){
//                 sameHight = false;
//                 break;
//             }
//         }
//         // if (sameHight)
//         //     chunks[chunk_id].m_Spheres.emplace_back(xValue, yValue, zValue);
//         return;
//     }
    
//     spheres.emplace_back(x_pos*sphereRadius, yValue*sphereRadius, z_pos*sphereRadius, sphereRadius);

//     // check 8 neighbour find the highest difference , [z][x]
//     int16_t maxDiff = 0;
//     for (int16_t k = 0; k < 8; k++){
//         int16_t difference = yValue - heightMap[z_pos+neighbs[k][0]][x_pos+neighbs[k][1]];
//         if (difference > maxDiff)
//             maxDiff = difference;
//     }

//     // if difference bigger than sphereRadius create more spheres to fill the gap
//     for (int16_t f = 2; f <= maxDiff; f += 2){
//         int32_t fillY = yValue - f;
//         spheres.emplace_back(x_pos*sphereRadius, fillY*sphereRadius, z_pos*sphereRadius, sphereRadius);
//     }
// }

// void ChunkHolder::generateSpheres() {
//     int32_t z_offset = 1;
//     int32_t x_offset = 1;

//     spheres.reserve(m_Width * m_Width * 4);
//     for (uint32_t i = 0; i < m_Width; i++){
//         // process chunks row by row
//         for (uint32_t j = 0; j < m_Width; j++){
//             generateSphere(j+x_offset, i+z_offset);
//         }
//     }
// }
