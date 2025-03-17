#ifndef CHUNK_HOLDER_HPP
#define CHUNK_HOLDER_HPP

#include "chunk.hpp"
#include "math/FastNoiseLite.hpp"

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