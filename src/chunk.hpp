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

struct ChunkPos { // 8 bytes
    int32_t x;
    int32_t z;

    ChunkPos() = default;
    ChunkPos(int32_t _x, int32_t _z) : x(_x), z(_z) {}
};

// uint32 data order is yVal + zOffset + xOffset
struct Sphere { // 4 bytes
    uint8_t xOffset;
    uint8_t zOffset;
    int16_t yVal;

    Sphere () = default;
    Sphere (uint8_t x, int16_t y, uint8_t z) : xOffset(x), yVal(y), zOffset(z) {}

    inline glm::vec3 getPosition(const ChunkPos &chunkPos, float radius){
        glm::vec3 result;
        result.x = (chunkPos.x + xOffset) * radius;
        result.y = yVal * radius;
        result.z = (chunkPos.z + zOffset) * radius;
        return result;
    } 
};

class Chunk {
public:
    Chunk() = default;

    // offset values cannot be bigger or equal than CHUNK_SIZE !
    // inline std::vector<Sphere>& GetCell(uint8_t x_off, uint8_t z_off) {
    //     return m_Grid[z_off*CHUNK_SIZE + x_off]; 
    // }

public:
    //std::array<std::vector<Sphere>, CHUNK_SIZE*CHUNK_SIZE> m_Grid;
    std::vector<Sphere> m_Spheres;
    BoundBox m_BoundBox;
    ChunkPos m_Position;

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
    inline Chunk& GetChunk(uint32_t x_off, uint32_t z_off){
        return chunks[z_off*chunkAmount + x_off];
    }

public: // variables
    std::vector<Chunk> chunks;
    

private: // variables
    std::vector<std::vector<int16_t>> heightMap; // 2D random Map Coords
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
    void generateChunk(const int32_t z_off, const int32_t x_off, uint32_t chunk_id);

    void generateHeightMap();
    void initNoise();
    float getTerrainHeight(float x, float z);
};



#endif