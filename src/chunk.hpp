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


// uint32 data order is yVal + zOffset + xOffset
struct Sphere { // 16 bytes
    glm::vec4 position;

    Sphere () = default;
    Sphere (float x, float y, float z, float rad) : position(x, y, z, rad){}
};

struct ChunkPos { // 8 bytes
    int32_t x;
    int32_t z;

    ChunkPos(int32_t x_, int32_t z_) : x(x_), z(z_) {}
    
    bool operator==(const ChunkPos &other) const {
        return x == other.x && z == other.z;
    }
};

class Chunk { // 48 bytes
public:
    Chunk(const int32_t x, const int32_t z);

    // offset values cannot be bigger or equal than CHUNK_SIZE !
    // inline std::vector<Sphere>& GetCell(uint8_t x_off, uint8_t z_off) {
    //     return m_Grid[z_off*CHUNK_SIZE + x_off]; 
    // }

public:
    //std::array<std::vector<Sphere>, CHUNK_SIZE*CHUNK_SIZE> m_Grid;
    std::vector<Sphere> spheres; // 24 bytes
    ChunkPos startPosition; // start position of the chunk x and z
    BoundBox boundBox; // 24 bytes
    uint32_t bufferVertexOffset; // location in vertex buffer
};


class HeightMapChunk { // CHUNK_SIZE * CHUNK_SIZE * 2 bytes
public:
    HeightMapChunk(){
        HeightMap = new int16_t[(CHUNK_SIZE + 2) * (CHUNK_SIZE + 2)];
    }
    ~HeightMapChunk(){
        delete[] HeightMap;
    }

    // z and x range in CHUNK_SIZE
    inline int16_t getHeight(int32_t z, int32_t x){
        // the range is [-1, CHUNK_SIZE] -> [0, CHUNK_SIZE+1]
        x += 1;
        z += 1;
        return HeightMap[z*(CHUNK_SIZE+2) + x];
    }
    // z and x range in CHUNK_SIZE
    inline void setHeight(int32_t z, int32_t x, int16_t val){
        // the range is [-1, CHUNK_SIZE] -> [0, CHUNK_SIZE+1]
        x += 1;
        z += 1;
        HeightMap[z*(CHUNK_SIZE+2) + x] = val;
    }
private:
    int16_t *HeightMap; // 2D int16_t map
};

class ChunkHolder {
public: // functions
    ChunkHolder(uint32_t chunk_am, float sphere_radius);
    
    inline uint32_t GetWidthChunkAmount() {
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
    //std::vector<Sphere> spheres;

private: // variables
    HeightMapChunk chunkHeightMap;
    uint32_t chunkAmount; //  total chunk size is chunkSize * chunkSize
    uint32_t totalNumOfSpheres;
    float sphereRadius;

    // noise variables
    FastNoiseLite base_noise;
    FastNoiseLite detail_noise;
    FastNoiseLite mountain_noise;
    FastNoiseLite terrain_mask;
    bool noise_initialized = false;
private: // functions
    void generateChunks();
    void generateChunk(uint32_t chunk_id);
    void generateHeightMapForChunk(const ChunkPos chunk_pos);

    void initNoise();
    float getTerrainHeight(float x, float z);
};



#endif