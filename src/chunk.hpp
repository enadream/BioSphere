#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <stdint.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <algorithm>

#include "bound_box.hpp"

// chunks will be square with this value 32*32
#define CHUNK_SIZE 16
// the deepness of indiviual quad 16*16 * 4
#define CHUNK_QUAD_DEEPNESS 1

struct LightVector { // 3 bytes
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;
    LightVector() : Red(0), Green(0), Blue(0){}
    LightVector(uint8_t red_, uint8_t green_, uint8_t blue_) : Red(red_), Green(green_), Blue(blue_) { }
    LightVector& operator=(const LightVector &other){
        Red = other.Red;
        Green = other.Green;
        Blue = other.Blue;
        return *this;
    }
};

struct CPU_Sphere { // 26 bytes
    uint16_t ChunkTypeAndFlags; // 4 bits free
    uint16_t AmbientOcclusion; // 4 bits free
    LightVector Lights[6]; // 6 direction +- (x,y,z)
    int16_t discHeight; // discrete height
    uint16_t posInChunk; // position in chunk 0 - 255

    CPU_Sphere(uint8_t x, int16_t y, uint8_t z) : ChunkTypeAndFlags(0), AmbientOcclusion(0){
        posInChunk = z*CHUNK_SIZE + x;
        discHeight = y;
    }
    inline void UpdatePos(uint8_t x, int16_t y, uint8_t z){
        posInChunk = z*CHUNK_SIZE + x;
        discHeight = y;
    }
    inline void GetPos(int16_t &x, int16_t &y, int16_t &z){
        y = discHeight;
        z = posInChunk / CHUNK_SIZE;
        x = posInChunk % CHUNK_SIZE;
    }
};

struct GPU_Sphere { // 38 bytes + 2 padding = 40 bytes in total
    glm::vec4 Position; // 16 bytes
    uint16_t ChunkTypeAndFlags; 
    uint16_t AmbientOcclusion;
    LightVector Lights[6]; // 18 bytes (6 * 3 bytes)
    uint16_t padding;
};

// Check if the size of the struct is correct.
static_assert(sizeof(GPU_Sphere) == 40, "GPU_Sphere struct size must be 40 bytes due to padding.");

struct ChunkPos { // 8 bytes
    int32_t x;
    int32_t z;

    ChunkPos(int32_t x_, int32_t z_) : x(x_), z(z_) {}
    
    bool operator==(const ChunkPos &other) const {
        return x == other.x && z == other.z;
    }
};

struct VertexInfo { // 8 bytes
    // starting of the vertex
    uint32_t start;
    // amount of spheres in this chunk
    uint32_t count;
};

class Chunk {
public:
    Chunk(const int32_t x, const int32_t z);

    // accomplishes binary search and outs offset and size
    bool GetCell(uint16_t x, uint16_t z, uint32_t &out_offset, uint32_t &out_size);
    bool GetSphere(int16_t x, int16_t y, int16_t z, uint32_t &out_index);
    bool InsertSphere(CPU_Sphere &sphere);
    bool DeleteSphere(uint16_t x, uint16_t z, int16_t y);
    void PrintSpheres();
    void GetGPUArray(std::vector<GPU_Sphere> &gpu_spheres, const float radius);
    
    // delete sphere using index
    inline void DeleteSphereWIndex(uint32_t index){
        spheres.erase(spheres.begin()+index);
    }
public:
    std::vector<CPU_Sphere> spheres; // 40 bytes
    ChunkPos startPosition; // 8 bytes, start position of the chunk x and z
    VertexInfo vertexInfo; // 8 bytes, position in the gpu
    BoundBox boundBox; // 24 bytes
};


#endif