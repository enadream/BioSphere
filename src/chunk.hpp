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
#define CHUNK_SIZE 32
// the deepness of indiviual quad 16*16 * 4
#define CHUNK_QUAD_DEEPNESS 1

struct ChunkInfo { // 4 + 4 + 8 + 24 = 40 bytes
    uint32_t offset;
    uint32_t size;
    glm::ivec2 pos;
    BoundBox boundBox; // 24 bytes
};

// uint32 data order is yVal + zOffset + xOffset
struct Sphere { // 16 bytes
    glm::ivec3 position;
    float radius;

    Sphere () = default;
    Sphere (int32_t x, int32_t y, int32_t z, float rad) : position(x, y, z), radius(rad) {}

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
    ChunkInfo m_ChunkInfo;
};

class ChunkHolder {
public: // functions
    ChunkHolder(int32_t width, float sphere_radius);
    
    inline uint32_t GetWidth() {
        return m_Width;
    }
    // inline uint32_t GetVertChunkAmount() {
    //     return chunkAmount;
    // }
    // inline uint32_t GetTotalChunkAmount() {
    //     return chunkAmount * chunkAmount;
    // }
    // inline uint32_t GetTotalNumOfSpheres(){
    //     return totalNumOfSpheres;
    // }
    // inline float GetSphereRadius(){
    //     return sphereRadius;
    // }
    // inline Chunk& GetChunk(uint32_t x_off, uint32_t z_off){
    //     return chunks[z_off*chunkAmount + x_off];
    // }

public: // variables
    //std::vector<Chunk> chunks;
    std::vector<Sphere> spheres;

private: // variables
    std::vector<std::vector<int16_t>> heightMap; // 2D random Map Coords
    uint32_t m_Width; //  total chunk size is chunkSize * chunkSize
    float sphereRadius;

    // noise variables
    FastNoiseLite base_noise;
    FastNoiseLite detail_noise;
    FastNoiseLite mountain_noise;
    FastNoiseLite terrain_mask;
    bool noise_initialized = false;

private: // functions
    void generateSpheres();
    void generateSphere(const int32_t z_pos, const int32_t x_pos);
    //void generateChunks();
    //void generateChunk(const int32_t z_off, const int32_t x_off, uint32_t chunk_id);

    void generateHeightMap();
    void initNoise();
    float getTerrainHeight(float x, float z);
};



#endif