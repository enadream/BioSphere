#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <stdint.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <algorithm>

#include "bound_box.hpp"
#include "math/FastNoiseLite.hpp"

// chunks will be square with this value 32*32
#define CHUNK_SIZE 16
// the deepness of indiviual quad 16*16 * 4
#define CHUNK_QUAD_DEEPNESS 1

struct Sphere { // 4 bytes
    uint8_t xOffset;
    uint8_t zOffset;
    int16_t yOffset;
};

struct ChunkPos { // 16 bytes
    int64_t x;
    int64_t z;
};

struct Grid {
    std::array<std::vector<Sphere>, CHUNK_SIZE*CHUNK_SIZE> grid;
};

struct Chunk {
    std::vector<glm::vec3> m_Positions;
    BoundBox m_BoundBox;
    uint32_t m_StartIndex;
};

class ChunkHolder {
public: // functions
    ChunkHolder(uint32_t chunk_am, float sphere_radius);
    
    inline uint32_t GetVertChunkAmount() {
        return chunkAmount;
    }
    inline uint32_t GetTotalChunkAmount() {
        return chunkAmount * chunkAmount;
    }
    inline uint32_t GetTotalNumOfSpheres(){
        return totalNumOfSpheres;
    }
    inline float GetSphereRadius(){
        return sphereRadius;
    }

public: // variables
    std::vector<Chunk> chunks;
    

private: // variables
    std::vector<std::vector<float>> heightMap; // 2D random Map Coords
    uint32_t totalNumOfSpheres;
    uint32_t chunkAmount; //  total chunk size is chunkSize * chunkSize
    float sphereRadius;

    // noise variables
    FastNoiseLite base_noise;
    FastNoiseLite detail_noise;
    FastNoiseLite mountain_noise;
    FastNoiseLite terrain_mask;
    bool noise_initialized = false;

private: // functions
    void generateChunks();
    void generateChunk(const uint32_t z_off, const uint32_t x_off, uint32_t chunk_id);

    void generateHeightMap();
    void initNoise();
    float getTerrainHeight(float x, float z);
};



#endif