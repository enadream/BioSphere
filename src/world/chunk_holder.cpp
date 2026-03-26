#include "world/chunk_holder.hpp"
#include "world/chunk.hpp"
#include "world/terrain_generator.hpp"
#include <cstdint>


ChunkHolder::ChunkHolder(uint32_t renderDistance, const Seed256& seed)
    : m_RenderDistance(renderDistance), m_TerrainGen(seed)
{
    GenerateWorld();
}

void ChunkHolder::GenerateWorld() {
    m_Chunks.clear();
    m_Chunks.reserve(m_RenderDistance * m_RenderDistance);
    m_CachedSphereCount = 0;
    
    // To center the 0,0 origin to the map
    ChunkCoordinates StartOffset(-(m_RenderDistance/2)*CHUNK_SIZE, -(m_RenderDistance/2)*CHUNK_SIZE);

    // Create chunks in a grid
    for (uint32_t z = 0; z < m_RenderDistance; z++) {
        for (uint32_t x = 0; x < m_RenderDistance; x++) {
            // World Coordinates for the chunk start (e.g., 0, 16, 32...)
            int32_t worldX = (int32_t)x * CHUNK_SIZE + StartOffset.X;
            int32_t worldZ = (int32_t)z * CHUNK_SIZE + StartOffset.Z;
            m_Chunks.emplace_back(worldX, worldZ);

            GenerateSingleChunk(m_Chunks.back());
        }
    }


    CalculateAmbientOcclusion(m_Chunks.back());
}

void ChunkHolder::GenerateSingleChunk(Chunk& chunk) {
    const ChunkCoordinates& coords = chunk.GetCoordinates();
    HeightMap heightMap;

    // Calculate the heightmap first
    GenerateHeightMap(coords, heightMap);

    // Set bounding box, because these are origins of spheres we need to add and subtract the radius to find BBox in x and z axes
    chunk.GetBounds().m_Min.z = SPHERE_RADIUS * (coords.Z - 1.0);
    chunk.GetBounds().m_Min.x = SPHERE_RADIUS * (coords.X - 1.0);
    chunk.GetBounds().m_Max.z = SPHERE_RADIUS * (coords.Z + CHUNK_SIZE);
    chunk.GetBounds().m_Max.x = SPHERE_RADIUS * (coords.X + CHUNK_SIZE);
    // Set the y value to the first value, this value can be updated with each sphere
    chunk.GetBounds().m_Min.y = SPHERE_RADIUS * (heightMap.GetHeight(0, 0) - 1);
    chunk.GetBounds().m_Max.y = SPHERE_RADIUS * (heightMap.GetHeight(0, 0) + 1);

    for (int32_t z = 0; z < CHUNK_SIZE; z++){ // iterate over z axis
        for (int32_t x = 0; x < CHUNK_SIZE; x++){ // iterate over x axis
            // 8 neighbours (z, x); up, left, rigt, down, diagonals
            constexpr int8_t neighbs[8][2] = {
                {-1, 0}, {0, -1}, {0, 1}, {1, 0}, // up, left, right, down
                {-1, -1}, {-1, 1}, {1, -1}, {1, 1} // top left, top right, bottom left, bottom right 
            };

            if ((z+x)%2 == 1){ // odd layer
                // Check if 4 neighbour if they have same hight value then you don't need to append this sphere
                int16_t upLevel = heightMap.GetHeight(z-1, x);
                bool sameHight = true;

                for (int32_t k = 1; k < 4; k++){
                    if (upLevel != heightMap.GetHeight(z+neighbs[k][0], x+neighbs[k][1])){
                        sameHight = false;
                        break;
                    }
                }
                if (sameHight) continue;
            }
            
            // Create sphere and add to chunk with an order
            SphereData sphere((uint8_t)x, heightMap.GetHeight(z, x), (uint8_t)z);
            chunk.GetSpheres().emplace_back(sphere);
            float worldY = sphere.DiscreteHeight * SPHERE_RADIUS;
            chunk.GetBounds().m_Min.y = glm::min(worldY-SPHERE_RADIUS, chunk.GetBounds().m_Min.y);
            chunk.GetBounds().m_Max.y = glm::max(worldY+SPHERE_RADIUS, chunk.GetBounds().m_Max.y);
            
            // Check 8 neighbour find the highest difference [z][x]
            int32_t maxDiff = 0;
            for (const auto& couple : neighbs){
                int32_t difference = sphere.DiscreteHeight - heightMap.GetHeight(z+couple[0], x+couple[1]);
                if (difference > maxDiff) maxDiff = difference;
            }

            // If difference bigger than sphereRadius create more spheres to fill the gap
            // If the max difference is odd like 3 we can place sphere to the position -2y
            // However if the difference is even like 4 we can't place sphere the -4y position cuz there is already a sphere exist
            // Instead filling the gap with odd layer would be better 
            uint32_t lastIndex = chunk.GetSpheres().size()-1; // get the last index
            int16_t fillY = sphere.DiscreteHeight;
            for (int16_t f = 2; f < maxDiff; f += 2){
                fillY = sphere.DiscreteHeight - f;
                // because the order of the spheres has to be ascending by height we need to insert smallest hight first
                chunk.GetSpheres().emplace(chunk.GetSpheres().begin()+lastIndex, (uint8_t)x, fillY, (uint8_t)z);
            }

            // check minimum y value again
            chunk.GetBounds().m_Min.y = glm::min(SPHERE_RADIUS * (fillY - 1), chunk.GetBounds().m_Min.y);
        }
    }

    m_CachedSphereCount += chunk.GetSpheres().size();
}

void ChunkHolder::CalculateAmbientOcclusion(uint32_t chunkId) {
    
    // This requires accessing neighbors.
    // In a professional engine, you'd use a Neighbor Lookup Table or Map.
    // For now, leaving empty or migrating your logic here.
}

void ChunkHolder::GenerateHeightMap(ChunkCoordinates chunkCoord, HeightMap &outHeightMap) {
    // generate [-1+x, x+1] height map

    // find terrain discrete hight map
    for (int32_t z = -1; z < CHUNK_SIZE+1; z++){
        float zWorld = (z+chunkCoord.Z) * SPHERE_RADIUS;
        
        for (int32_t x = -1; x < CHUNK_SIZE+1; x++){
            float xWorld = (x+chunkCoord.X) * SPHERE_RADIUS;
            float height = m_TerrainGen.GetHeight(xWorld , zWorld);
            int16_t discreteY = static_cast<int16_t>(height/SPHERE_RADIUS);

            // even layers like (x+z)=0,2,4 can have even number of multiples of sphereRadius
            // however odd layers has to be odd number of multiples of sphereRadius 
            discreteY -= (z + x + discreteY) % 2;
            // store the data to the chunk map
            outHeightMap.SetHeight(z, x, discreteY);
        }
    }
}
