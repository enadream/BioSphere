#include "compute_handler.hpp"
#include <stdio.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

ComputeConfig::ComputeConfig() : LocalSize(0), DispatchSize(0){}

void ComputeConfig::GetSetLimits(){
   // max number of compute shader work groups
   glGetInteger64i_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkGC.x);
   glGetInteger64i_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxWorkGC.y);
   glGetInteger64i_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWorkGC.z);

   // max number of compute shader local groups
   glGetInteger64i_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupSize.x);
   glGetInteger64i_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkGroupSize.y);
   glGetInteger64i_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkGroupSize.z);
   
   // max number of invocations
   glGetInteger64v(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkGroupInvocations);
}

void ComputeConfig::CalculateSize(int64_t total_count, int32_t idealLocalSizeX){
    GetSetLimits();

    uint32_t localSizeX = std::min({ maxWorkGroupSize.x, maxWorkGroupInvocations, (int64_t)idealLocalSizeX});
    LocalSize = { localSizeX, 1, 1 }; // Using a 1D layout for simplicity

    // Determine how many workgroups (in 1D) are needed to cover TotalCount operations.
    int64_t groupsNeeded = (localSizeX + total_count - 1) / localSizeX; // ceiling division

    // Now distribute these groups over x, y, and z while respecting hardware limits.
    // First, use as many groups in x as allowed.
    int64_t dispatchX  = std::min(groupsNeeded, maxWorkGC.x);

    // The remaining groups will be divided into y and then z dimensions.
    int64_t remainingGroups = (groupsNeeded + dispatchX - 1) / dispatchX; // ceil(groupsNeeded / dispatchX)
    int64_t dispatchY = std::min(remainingGroups, maxWorkGC.y);

    remainingGroups = (remainingGroups + dispatchY - 1) / dispatchY;
    int64_t dispatchZ = std::min(remainingGroups, maxWorkGC.z);
    
    DispatchSize = {dispatchX, dispatchY, dispatchZ};

    // If the product of dispatch dimensions doesn't cover groupsNeeded, warn the user.
    if (dispatchX * dispatchY * dispatchZ < groupsNeeded) {
        printf("[ERROR]: The computed dispatch size (%llix%llix%lli) is less than required (%lli groups). Not all operations may be dispatched.\n",
            dispatchX, dispatchY, dispatchZ, groupsNeeded);
    }
}

void ComputeConfig::PrintSizes(){
    printf("---------------------\n");
    printf("[INFO]: Local Size : %u,%u,%u\n", LocalSize.x, LocalSize.y, LocalSize.z);
    printf("[INFO]: Dispatch Size : %u,%u,%u\n", DispatchSize.x, DispatchSize.y, DispatchSize.z);
    printf("--------Limits-------\n");
    printf("[INFO]: Max Work Group Count: %lli, %lli, %lli\n", maxWorkGC.x, maxWorkGC.y, maxWorkGC.z);
    printf("[INFO]: Max Work Group Size: %lli, %lli, %lli\n", maxWorkGroupSize.x, maxWorkGroupSize.y, maxWorkGroupSize.z);
    printf("[INFO]: Max Work Group Invocations: %lli\n", maxWorkGroupInvocations);
    printf("---------------------\n");
}

void ComputeConfig::PrintLimits(){
    // max number of vertex attributes
    int nrAttributes, shaderStorageBufferBindings;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    
    // max number of shader storage buffer bindings
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &shaderStorageBufferBindings);

    // max block size for ssbo
    GLint64 maxSSBOSize = 0;
    glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxSSBOSize);

    // max number of compute shader work groups
    int32_t maxWorkGC[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkGC[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxWorkGC[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWorkGC[2]);

    // max number of compute shader local groups
    int32_t maxWorkGroupSize[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupSize[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkGroupSize[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkGroupSize[2]);
    
    // max number of invocations
    int32_t maxWorkGroupInvocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkGroupInvocations);
    
    float pointSizeRange[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, pointSizeRange);

    printf("Maximum nr of vertex attributes supported: %i \n", nrAttributes);
    printf("Maximum nr of shader storage buffer bindings: %i \n", shaderStorageBufferBindings);
    printf("Maximum size of SSBO block size: %lli \n", maxSSBOSize);
    printf("Max Work Group Count: %i, %i, %i\n", maxWorkGC[0], maxWorkGC[1], maxWorkGC[2]);
    printf("Max Work Group Size: %i, %i, %i\n", maxWorkGroupSize[0], maxWorkGroupSize[1], maxWorkGroupSize[2]);
    printf("Max Work Group Invocations: %i\n", maxWorkGroupInvocations);
    printf("Point size range %f, %f\n", pointSizeRange[0],pointSizeRange[1]);
}