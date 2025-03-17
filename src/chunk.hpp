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

struct LightVector { // 3 bytes
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;
};

struct CPU_Sphere { // 26 bytes
    uint16_t ChunkTypeAndFlags; // 4 bits free
    uint16_t AmbientOcclusion; // 4 bits free
    LightVector Lights[6]; // 6 direction +- (x,y,z)
    int16_t discHeight; // discrete height
    uint16_t posInChunk; // position in chunk 0 - 255
};

struct GPU_Sphere { // 40 bytes
    glm::vec4 position; // 16 bytes
    uint16_t ChunkTypeAndFlags; 
    uint16_t AmbientOcclusion;
    LightVector Lights[6]; // 18 (3*6) bytes
    uint8_t padding;
};

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

public:

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




#endif