#ifndef COMPUTE_HANDLER_HPP
#define COMPUTE_HANDLER_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <stdint.h>

// Structure holding the compute shader configuration.
class ComputeConfig {
public:
    ComputeConfig();
    void CalculateSize(int64_t total_count, int32_t idealLocalSizeX);

    void PrintSizes();
    static void PrintLimits();

public:
    glm::uvec3 LocalSize;    // Workgroup (layout) size
    glm::uvec3 DispatchSize; // Number of workgroups to dispatch
    
private:
    // Sets the limits from gpu 
    void GetSetLimits();
    glm::i64vec3 maxWorkGC;
    glm::i64vec3 maxWorkGroupSize;
    int64_t maxWorkGroupInvocations;
};



#endif