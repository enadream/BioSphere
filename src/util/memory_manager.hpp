#pragma once

#include <stdint.h>
#include <vector>
#include <stdio.h>

struct MemBlock { // 8 bytes
    uint32_t begin;  // begging index
    uint32_t size;   // size of the free block

    MemBlock (uint32_t _begin, uint32_t _size) : begin(_begin), size(_size) {}
};

class MemoryManager {
public: // functions
    MemoryManager(uint32_t total_size);
    
    // request a location with size
    bool Allocate(uint32_t size, uint32_t &out_index);
    // you can free only allocated spaces!!!
    void Free(uint32_t begin_id, uint32_t size);
    // prints the free blocks in memory
    void PrintFreeBlocks();

    // clears freeBlocks and reset totalSize
    void ResetMemory(uint32_t total_size);

    inline uint32_t GetTotalSize(){
        return totalSize;
    }

private: // variables
    uint32_t totalSize;
    std::vector<MemBlock> freeBlocks;

private: // functions

};
