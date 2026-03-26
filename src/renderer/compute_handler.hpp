#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>

// Helper to calculate and manage Compute Shader dispatch sizes
class ComputeDispatcher {
public:
    // Call this once at startup (after context creation) to query GPU limits
    static void Init();

    // Calculates optimal dispatch dimensions for a 1D problem (e.g., array of particles)
    // - totalCount: Total number of items to process
    // - localSizeX: The local_size_x defined in your shader (usually 32, 64, or 128)
    static glm::uvec3 CalculateDispatchSize1D(uint64_t totalCount, uint32_t localSizeX);

    // Prints cached hardware limits to the console
    static void LogLimits();

private:
    struct ComputeLimits {
        glm::i32vec3 MaxWorkGroupCount;       // GL_MAX_COMPUTE_WORK_GROUP_COUNT
        glm::i32vec3 MaxWorkGroupSize;        // GL_MAX_COMPUTE_WORK_GROUP_SIZE
        int32_t MaxWorkGroupInvocations;      // GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
        int64_t MaxSSBOSize;                  // GL_MAX_SHADER_STORAGE_BLOCK_SIZE
        bool Initialized = false;
    };

    static ComputeLimits s_Limits;
};