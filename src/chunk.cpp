#include "chunk.hpp"

Chunk::Chunk(const int32_t x, const int32_t z) : startPosition(x, z), vertexInfo(0, 0) {}

bool Chunk::GetCell(uint16_t x, uint16_t z, uint32_t &out_offset, uint32_t &out_size){
    uint16_t targetPosInChunk = z*CHUNK_SIZE + x;
    // binary search
    int32_t left = 0;
    int32_t right = spheres.size() - 1;

    while (left <= right){
        int32_t mid = left + (right - left)/2;

        if (spheres[mid].posInChunk == targetPosInChunk){
            uint32_t offset = mid;
            uint32_t end_index = mid;
            // find the offset point
            for (int32_t i = mid-1; i > -1 && spheres[i].posInChunk == targetPosInChunk; i--){
                offset = i;
            }
            // find the end point
            for (int32_t i = mid+1; (uint64_t)i < spheres.size() && spheres[i].posInChunk == targetPosInChunk; i++){
                end_index = i;
            }
            out_offset = offset;
            out_size = end_index - offset + 1;
            return true;
        }
        else if (spheres[mid].posInChunk > targetPosInChunk){
            // search in left half
            right = mid-1;
        }
        else { // search in right half
            left = mid+1;
        }
    }
    printf("[WARNING]: Target cell couldn't found! \n");
    printf("\tTarget Cell = z:%hu x:%hu\n", z, x);
    return false;
}

bool Chunk::GetSphere(int16_t x, int16_t y, int16_t z, uint32_t &out_index){
    uint16_t targetPosInChunk = z*CHUNK_SIZE + x;
    // binary search
    int32_t left = 0;
    int32_t right = spheres.size() - 1;

    while (left <= right){
        int32_t mid = left + (right - left)/2;

        if (spheres[mid].posInChunk == targetPosInChunk){
            if (spheres[mid].discHeight == y){
                out_index = mid;
                return true;
            }
            else if (spheres[mid].discHeight > y){
                // search in left side
                for (int32_t i = mid-1; i > -1 && spheres[i].posInChunk == targetPosInChunk; i--){
                    if (spheres[i].discHeight == y){
                        out_index = i;
                        return true;
                    }
                }
            }
            else {
                // search in right side
                for (int32_t i = mid+1; (uint64_t)i < spheres.size() && spheres[i].posInChunk == targetPosInChunk; i++){
                    if (spheres[i].discHeight == y){
                        out_index = i;
                        return true;
                    }
                }
            }

            return false;
        }
        else if (spheres[mid].posInChunk > targetPosInChunk){
            // search in left half
            right = mid-1;
        }
        else { // search in right half
            left = mid+1;
        }
    }
    return false;
}

bool Chunk::InsertSphere(CPU_Sphere &new_sphere){
    uint16_t targetPosInChunk = new_sphere.posInChunk;
    // accomplish binary search to find lowerbound
    int32_t left = 0;
    int32_t right = spheres.size() - 1;
    uint32_t targetIndex = spheres.size();

    while (left <= right){
        int32_t mid = left + (right - left)/2;

        if (spheres[mid].posInChunk >= targetPosInChunk){
            targetIndex = mid;
            right = mid-1;
        }
        else {
            left = mid+1;
        }
    }

    // iterate through the same cell
    while (targetIndex < spheres.size() && spheres[targetIndex].posInChunk == targetPosInChunk)
    {
        if (spheres[targetIndex].discHeight < new_sphere.discHeight){
            targetIndex++;
        }
        else if (spheres[targetIndex].discHeight == new_sphere.discHeight){
            uint16_t z = new_sphere.posInChunk / CHUNK_SIZE;
            uint16_t x = new_sphere.posInChunk % CHUNK_SIZE;
            printf("[WARNING]: Sphere insert failed, you two spheres cannot have same hight in same cell! \n");
            printf("\tNew Sphere = z:%hu x:%hu y:%hi\n", z, x, new_sphere.discHeight);
            return false;
        }
        else {
            break;
        }
    }
    // insert
    spheres.emplace(spheres.begin()+targetIndex, new_sphere);
    return true;
}

bool Chunk::DeleteSphere(uint16_t x, uint16_t z, int16_t y){
    uint16_t targetPosInChunk = z*CHUNK_SIZE + x;
    int32_t left = 0;
    int32_t right = spheres.size() - 1;

    while (left <= right){
        int32_t mid = left + (right-left)/2;

        if (spheres[mid].posInChunk == targetPosInChunk){
            // search target y
            for (int32_t i = mid; i > -1 && spheres[i].posInChunk == targetPosInChunk; i--){
                if (spheres[i].discHeight == y){
                    spheres.erase(spheres.begin()+i);
                    return true;
                }
            }
            for (int32_t i = mid; (uint64_t)i < spheres.size() && spheres[i].posInChunk == targetPosInChunk; i++){
                if (spheres[i].discHeight == y){
                    spheres.erase(spheres.begin()+i);
                    return true;
                }
            }
            break;
        }
        else if (spheres[mid].posInChunk > targetPosInChunk){
            right = mid-1;
        }
        else {
            left = mid+1;
        }

    }
    printf("[WARNING]: Target sphere couldn't found, No deletion operation accomplished! \n");
    printf("\tTarget Sphere = z:%hu x:%hu y:%hi\n", z, x, y);
    return false;
}

void Chunk::GetGPUArray(std::vector<GPU_Sphere> &gpu_spheres, const float radius){
    gpu_spheres.reserve(spheres.size());

    for (uint32_t i = 0; i < spheres.size(); i++){
        GPU_Sphere gpuSphere;

        float posZ = (startPosition.z + (spheres[i].posInChunk/CHUNK_SIZE))*radius;
        float posX = (startPosition.x + (spheres[i].posInChunk%CHUNK_SIZE))*radius;
        float posY = spheres[i].discHeight*radius;

        gpuSphere.Position = glm::vec4(posX, posY, posZ, radius);
        gpuSphere.AmbientOcclusion = spheres[i].AmbientOcclusion;
        gpuSphere.ChunkTypeAndFlags = spheres[i].ChunkTypeAndFlags;
        for (uint8_t l = 0; l < 6; l++){
            gpuSphere.Lights[l] = (spheres[i].Lights[l]);
        }

        gpu_spheres.emplace_back(gpuSphere);
    }
}

void Chunk::PrintSpheres(){
    printf("Total Num Of Spheres: %llu\n", spheres.size());
    for (uint32_t i = 0; i < spheres.size(); i++){
        uint16_t z = spheres[i].posInChunk / CHUNK_SIZE;
        uint16_t x = spheres[i].posInChunk % CHUNK_SIZE;

        printf("%u. pos in chunk z:%hu, x:%hu, y:%hi\n", i, z, x, spheres[i].discHeight);
    }
}

// int main (){
//     Chunk chunk(0,0);
//     CPU_Sphere sphere(0,0,5);
//     chunk.InsertSphere(sphere);
    
//     sphere.UpdatePos(2,2,-5);
//     chunk.InsertSphere(sphere);
//     sphere.UpdatePos(0,0,-5);
//     chunk.InsertSphere(sphere);
//     sphere.UpdatePos(0,0,-9);
//     chunk.InsertSphere(sphere);
//     sphere.UpdatePos(4,4,1);
//     chunk.InsertSphere(sphere);
//     sphere.UpdatePos(2,1,2);
//     chunk.InsertSphere(sphere);
//     sphere.UpdatePos(2,1,3);
//     chunk.InsertSphere(sphere);
//     sphere.UpdatePos(2,1,4);
//     chunk.InsertSphere(sphere);
//     sphere.UpdatePos(2,1,5);
//     chunk.InsertSphere(sphere);
//     sphere.UpdatePos(2,1,-5);
//     chunk.InsertSphere(sphere);
//     sphere.UpdatePos(1,0,9);
//     chunk.InsertSphere(sphere);
//     chunk.PrintSpheres();

//     uint32_t offset, size;
//     bool result = chunk.GetSphere(0, -7, 0, offset);

//     if (result)
//         printf("Index:%u\n", offset);
// }