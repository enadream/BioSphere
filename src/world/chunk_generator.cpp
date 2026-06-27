#include "world/chunk_generator.hpp"
#include "world/chunk.hpp"

ChunkGenerator::ChunkGenerator(const Seed256& seed) : m_TerrainGen(seed) {}

void ChunkGenerator::Generate(Chunk& chunk) const {
    HeightMap heightMap;
    GenerateHeightMap(chunk.GetCoordinates(), heightMap);
    InitBounds(chunk, heightMap);
    FillChunk(chunk, heightMap);
}

void ChunkGenerator::GenerateHeightMap(const ChunkCoordinates& coords, HeightMap& outMap) const {
    // generate [-1+x, x+1] height map

    // find terrain discrete hight map
    for (int32_t z = -1; z < CHUNK_SIZE+1; z++){
        float zWorld = (z + coords.Z * static_cast<int32_t>(CHUNK_SIZE)) * SPHERE_RADIUS;

        for (int32_t x = -1; x < CHUNK_SIZE+1; x++){
            float xWorld = (x + coords.X * static_cast<int32_t>(CHUNK_SIZE)) * SPHERE_RADIUS;
            float height = m_TerrainGen.GetHeight(xWorld , zWorld);
            int16_t discrY = static_cast<int16_t>(height / SPHERE_RADIUS);

            // even layers like (x+z)=0,2,4 can have even number of multiples of sphereRadius
            // however odd layers has to be odd number of multiples of sphereRadius 
            discrY -= static_cast<int16_t>((z + x + discrY) % 2);
            outMap.SetHeight(z, x, discrY);
        }
    }
}

void ChunkGenerator::InitBounds(Chunk& chunk, const HeightMap& heightMap) const {
    const ChunkCoordinates& coords = chunk.GetCoordinates();
    const int32_t baseX = coords.X * static_cast<int32_t>(CHUNK_SIZE);
    const int32_t baseZ = coords.Z * static_cast<int32_t>(CHUNK_SIZE);

    chunk.GetBounds().m_Min.x = SPHERE_RADIUS * static_cast<float>(baseX - 1);
    chunk.GetBounds().m_Min.z = SPHERE_RADIUS * static_cast<float>(baseZ - 1);
    chunk.GetBounds().m_Max.x = SPHERE_RADIUS * static_cast<float>(baseX + static_cast<int32_t>(CHUNK_SIZE));
    chunk.GetBounds().m_Max.z = SPHERE_RADIUS * static_cast<float>(baseZ + static_cast<int32_t>(CHUNK_SIZE));

    // Initialise Y bounds from cell (0,0) -- FillChunk will expand them
    const float firstY         = heightMap.GetHeight(0, 0) * SPHERE_RADIUS;
    chunk.GetBounds().m_Min.y  = firstY - SPHERE_RADIUS;
    chunk.GetBounds().m_Max.y  = firstY + SPHERE_RADIUS;
}


void ChunkGenerator::FillChunk(Chunk& chunk, const HeightMap& heightMap) const {
    // 8 neighbours (z, x); up, left, rigt, down, diagonals
    constexpr int8_t NEIGHBOURS[8][2] = {
        {-1,  0}, { 0, -1}, { 0,  1}, { 1,  0},  // cardinal // up, left, right, down
        {-1, -1}, {-1,  1}, { 1, -1}, { 1,  1}   // diagonal // top left, top right, bottom left, bottom right 
    };

    chunk.GetSpheres().reserve(CHUNK_SIZE * CHUNK_SIZE * 2);

    for (int32_t z = 0; z < CHUNK_SIZE; z++){ // iterate over z axis
        for (int32_t x = 0; x < CHUNK_SIZE; x++){ // iterate over x axis
            const int16_t centerY = heightMap.GetHeight(z, x);

            // Odd-layer optimisation: skip if all 4 cardinal neighbours share this height
            if ((z + x) % 2 == 1) {
                const int16_t refY = heightMap.GetHeight(z + NEIGHBOURS[0][0], x + NEIGHBOURS[0][1]);
                bool sameHight = true;

                // Check if 4 neighbour if they have same hight value then you don't need to append this sphere
                for (int16_t k = 1; k < 4; k++){
                    if (refY != heightMap.GetHeight(z+NEIGHBOURS[k][0], x+NEIGHBOURS[k][1])){
                        sameHight = false;
                        break;
                    }
                }
                if (sameHight) continue;
            }

            // Fill every offset between the surface and the lowest cardinal neighbour so
            // adjacent column spheres overlap by 0.5 world units. Even offsets land on the
            // BCC lattice; odd ones do not - they exist purely to close the knife-edge gap
            // where two touching spheres meet at a single point (FP-precision pixel cracks).
            int16_t cardinalMaxDiff = 0;
            for (int8_t k = 0; k < 4; ++k) {
                const int16_t diff = centerY - heightMap.GetHeight(z + NEIGHBOURS[k][0], x + NEIGHBOURS[k][1]);
                if (diff > cardinalMaxDiff) cardinalMaxDiff = diff;
            }

            if (cardinalMaxDiff >= 2) {
                const int16_t minY = centerY - (cardinalMaxDiff - 1);
                chunk.GetBounds().m_Min.y = std::min(SPHERE_RADIUS * (minY - 1), chunk.GetBounds().m_Min.y);

                // Lowest first so the vector stays sorted ascending by height.
                for (int16_t f = cardinalMaxDiff - 1; f >= 1; --f) {
                    const int16_t fillY = centerY - f;
                    chunk.GetSpheres().emplace_back(static_cast<uint8_t>(x), fillY, static_cast<uint8_t>(z));
                }
            }
            
            // Insert the surface sphere
            chunk.GetSpheres().emplace_back(static_cast<uint8_t>(x), centerY, static_cast<uint8_t>(z));
            const float worldY = centerY * SPHERE_RADIUS;
            // update min max bound box values again
            chunk.GetBounds().m_Min.y = std::min(worldY - SPHERE_RADIUS, chunk.GetBounds().m_Min.y);
            chunk.GetBounds().m_Max.y = std::max(worldY + SPHERE_RADIUS, chunk.GetBounds().m_Max.y);
        }
    }
}