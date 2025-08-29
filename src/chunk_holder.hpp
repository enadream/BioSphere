#ifndef CHUNK_HOLDER_HPP
#define CHUNK_HOLDER_HPP

#include "chunk.hpp"
#include "math/FastNoiseLite.hpp"

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
    uint32_t chunkAmount; //  horizontal chunk amount
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
    void calcAmbientOcc(uint32_t chunk_id);
    void generateHeightMapForChunk(const ChunkPos chunk_pos);

    void initNoise();
    float getTerrainHeight(float x, float z);
};

#endif