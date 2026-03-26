#pragma once

#include "renderer/vertex_array.hpp"
#include <cstring>
#include <glm/glm.hpp>
#include <memory>
#include <glad/glad.h> // Required for GL commands inside header

// Stats to profile the indirect draws
struct RenderStats {
    uint32_t DrawCalls = 0;
    uint32_t IndirectDrawCount = 0;
    uint32_t VertexCount = 0;
    uint32_t IndexCount = 0;
};

class Renderer {
public:
    static void Init(bool debugEnabled = true);
    static void Shutdown();

    static void OnWindowResize(uint32_t width, uint32_t height) {
        glViewport(0, 0, width, height);
    }

    // State Commands
    static void SetClearColor(const glm::vec4& color) {
        glClearColor(color.r, color.g, color.b, color.a);
    }
    static void Clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    static void SetDepthTest(bool enabled) {
        if (enabled) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
    }
    static void SetCullFace(bool enabled) {
        if (enabled) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
    }
    static void SetWireframeMode(bool enabled) {
        if (enabled) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    static void SetProgramPointSize(bool enabled) {
        if (enabled) glEnable(GL_PROGRAM_POINT_SIZE);
        else glDisable(GL_PROGRAM_POINT_SIZE);
    }
    static void SetDebugMode(bool enabled) {
        if (enabled) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        } else {
            glDisable(GL_DEBUG_OUTPUT); 
        }
    }

    // Draw commands
    // Standard Draw (UI, Skybox, basic blocks)
    static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0);

    // Advanced Draw (For your Chunks/Spheres)
    // - vertexArray: Your "sphereVAO"
    // - indirectBuffer: Your "drawCommandBuffer" (Must be bound to GL_DRAW_INDIRECT_BUFFER)
    // - drawCount: How many commands are in the buffer
    // - stride: 0 usually (tightly packed)
    static void DrawMultiIndirect(const std::shared_ptr<VertexArray>& vertexArray, 
        uint32_t drawCount, uint32_t stride = 0);


    // --- Statistics ---
    static void ResetStats() { memset(&s_Stats, 0, sizeof(RenderStats)); }
    static RenderStats GetStats() { return s_Stats; }

private:
    static RenderStats s_Stats;
};