#include "renderer/compute_handler.hpp"
#include "core/log.hpp"

#include <algorithm>

// Initialize static member
ComputeDispatcher::ComputeLimits ComputeDispatcher::s_Limits;

void ComputeDispatcher::Init() {
    if (s_Limits.Initialized) return;

    // Query Work Group Count Limits
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &s_Limits.MaxWorkGroupCount.x);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &s_Limits.MaxWorkGroupCount.y);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &s_Limits.MaxWorkGroupCount.z);

    // Query Work Group Size Limits
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &s_Limits.MaxWorkGroupSize.x);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &s_Limits.MaxWorkGroupSize.y);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &s_Limits.MaxWorkGroupSize.z);

    // Query Invocations and SSBO size
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &s_Limits.MaxWorkGroupInvocations);
    glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &s_Limits.MaxSSBOSize);

    s_Limits.Initialized = true;
    
    // LogLimits(); // Optional: Log on init
}

glm::uvec3 ComputeDispatcher::CalculateDispatchSize1D(uint64_t totalCount, uint32_t localSizeX) {
    // Ensure limits are fetched
    if (!s_Limits.Initialized) Init();

    // Safety check for local size
    if (localSizeX > (uint32_t)s_Limits.MaxWorkGroupSize.x) {
        LOG_WARN("ComputeDispatcher: LocalSizeX (%d) exceeds Hardware Limit (%d). Clamping.", 
                 localSizeX, s_Limits.MaxWorkGroupSize.x);
        localSizeX = s_Limits.MaxWorkGroupSize.x;
    }

    // 1. Calculate total workgroups needed (Ceiling Division)
    // Formula: (N + GroupSize - 1) / GroupSize
    uint64_t groupsNeeded = (totalCount + localSizeX - 1) / localSizeX;

    // 2. Dispatch Logic
    // We try to fit everything in X. If X limit is exceeded, we wrap to Y, then Z.
    
    // X Dimension
    uint32_t dispatchX = (uint32_t)std::min((uint64_t)s_Limits.MaxWorkGroupCount.x, groupsNeeded);
    
    // Y Dimension (if needed)
    uint64_t remaining = (groupsNeeded + dispatchX - 1) / dispatchX;
    uint32_t dispatchY = (uint32_t)std::min((uint64_t)s_Limits.MaxWorkGroupCount.y, remaining);

    // Z Dimension (if needed)
    remaining = (remaining + dispatchY - 1) / dispatchY;
    uint32_t dispatchZ = (uint32_t)std::min((uint64_t)s_Limits.MaxWorkGroupCount.z, remaining);

    // Check if we still have groups left (Total job is too big for this GPU!)
    if (remaining > (uint64_t)s_Limits.MaxWorkGroupCount.z) {
        LOG_ERROR("ComputeDispatcher: Total work count too large for GPU! Processing will be incomplete.");
    }

    return glm::uvec3(dispatchX, dispatchY, dispatchZ);
}

void ComputeDispatcher::LogLimits() {
    if (!s_Limits.Initialized) Init();

    LOG_INFO("Compute Shader Capabilities:");
    LOG_INFO("  Max Work Groups: (%d, %d, %d)", 
             s_Limits.MaxWorkGroupCount.x, s_Limits.MaxWorkGroupCount.y, s_Limits.MaxWorkGroupCount.z);
    LOG_INFO("  Max Local Size:  (%d, %d, %d)", 
             s_Limits.MaxWorkGroupSize.x, s_Limits.MaxWorkGroupSize.y, s_Limits.MaxWorkGroupSize.z);
    LOG_INFO("  Max Invocations: %d", s_Limits.MaxWorkGroupInvocations);
    LOG_INFO("  Max SSBO Size:   %lld bytes", s_Limits.MaxSSBOSize);
}