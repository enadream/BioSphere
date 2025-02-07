#ifndef MEMORY_CONTROLLER_HPP
#define MEMORY_CONTROLLER_HPP

#include <stdint.h>
#include <vector>
#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "chunk.hpp"

struct ChunkInfo { // 16 bytes
    uint32_t offset; 
    uint32_t size;
    ChunkPos pos; // 8 bytes
    BoundBox m_BoundBox; // 24 bytes
};

class ChunkMemoryHandler {
public: // functions
    ChunkMemoryHandler();


public: // variables
    std::vector<ChunkInfo> m_ChunkRanges;
    
private: // variables
private: // functions

    // this function synchronizes cpu chunk info with buffer chunk info
    void SyncChunkBuffer(uint32_t target_chunk);
};


#endif