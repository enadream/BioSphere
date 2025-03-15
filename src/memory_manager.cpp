#include "memory_manager.hpp"

MemoryManager::MemoryManager(uint32_t total_size) : totalSize(total_size) {
    // emplace the free space
    freeBlocks.emplace_back(0, total_size);
}

// request a location with size
bool MemoryManager::Allocate(uint32_t size, uint32_t &out_index) {
    // search the first fit
    for (uint32_t i = 0; i < freeBlocks.size(); i++){
        if (size < freeBlocks[i].size){
            out_index = freeBlocks[i].begin;
            // increase beginning index
            freeBlocks[i].begin += size;
            // decrease size
            freeBlocks[i].size -= size;
        }
        else if (size == freeBlocks[i].size){ // exact fit
            out_index = freeBlocks[i].begin;
            // erase the free block
            freeBlocks.erase(freeBlocks.begin()+i);
            return true;
        }
    }
    return false; // there isn't enough storage
}

// you can free only the allocated spaces !!!
void MemoryManager::Free(uint32_t begin_id, uint32_t size){
    uint32_t end_id = begin_id + size;

    // the newly freed memory cannot overlap with existing free blocks!!!
    // new freed memory has to be in allocated space, 
    // the thing you should check now if is there any continuity in free spaces so you can merge them

    // target -> [8 10)
    // [5, 8), [10, 12)

    uint32_t insertIndex = 0;

    // find continuity if exists merge them
    for (uint32_t i = 0; i < freeBlocks.size(); i++){
        if (freeBlocks[i].begin < begin_id){
            insertIndex++;
        }

        uint32_t end = freeBlocks[i].begin + freeBlocks[i].size;
        if (end == begin_id){
            freeBlocks[i].size += size;

            // check the next free space
            if (i+1 < freeBlocks.size() && end_id == freeBlocks[i+1].begin){
                // increase the current block size
                freeBlocks[i].size += freeBlocks[i+1].size;
                // delete the next block
                freeBlocks.erase(freeBlocks.begin()+(i+1));
            }
            return;
        }
        else if (end_id == freeBlocks[i].begin){
            // change begin id to new free space
            freeBlocks[i].begin = begin_id;
            freeBlocks[i].size += size;
            return;
        }
    }
    // when there is no continuity found, insert the block to the target area
    freeBlocks.emplace(freeBlocks.begin()+insertIndex, begin_id, size);
}

// clears freeBlocks and reset totalSize
void MemoryManager::ResetMemory(uint32_t total_size){
    freeBlocks.clear();
    totalSize = total_size;
    freeBlocks.emplace_back(0, total_size);
}

void MemoryManager::PrintFreeBlocks(){
    for (uint32_t i = 0; i < freeBlocks.size(); i++){
        printf("%u.Range: [%u, %u), Size: %u\n", i, freeBlocks[i].begin, freeBlocks[i].begin+freeBlocks[i].size, freeBlocks[i].size);
    }
}

// testing the function
// int main() {
//     // target -> [8 10)
//     // [5, 8), [10, 12)
    
//     MemoryManager memManager(500);
//     uint32_t ot;

//     memManager.PrintFreeBlocks();
//     printf("\n");

//     memManager.Allocate(50, ot);
//     printf("out %u\n", ot);
//     memManager.PrintFreeBlocks();
//     printf("\n");

//     memManager.Free(5, 3);
//     memManager.PrintFreeBlocks();
//     printf("\n");
//     memManager.Free(10, 2);
//     memManager.PrintFreeBlocks();

//     printf("\n");
//     memManager.Free(8, 2);
//     memManager.PrintFreeBlocks();

//     printf("\n");
//     memManager.Allocate(6, ot);
//     memManager.PrintFreeBlocks();

//     printf("\n");
//     memManager.ResetMemory(100);
//     memManager.PrintFreeBlocks();
// }