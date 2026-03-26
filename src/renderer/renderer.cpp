#include "renderer/renderer.hpp"
#include "core/log.hpp"

#include <glad/glad.h>

RenderStats Renderer::s_Stats;

// --- OpenGL Debug Callback ---
void APIENTRY OpenGLMessageCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity, 
    GLsizei length, const GLchar* message, const void* userParam) 
{
    // Filter generic NVIDIA buffer info
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         LOG_FATAL("[OpenGL High] %s", message); break;
        case GL_DEBUG_SEVERITY_MEDIUM:       LOG_ERROR("[OpenGL Medium] %s", message); break;
        case GL_DEBUG_SEVERITY_LOW:          LOG_WARN("[OpenGL Low] %s", message); break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_TRACE("[OpenGL Note] %s", message); break;
    }
}

void Renderer::Init(bool debugEnabled){
    // 1. Setup Debugging Callback (Always setup, toggle via enable/disable)
    glDebugMessageCallback(OpenGLMessageCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    // Set initial state
    SetDebugMode(debugEnabled);

    // 2. Depth & Blending
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    //// Blending
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //// Polygon mode
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glEnable(GL_CULL_FACE);
    
    // 3. Point Size
    glEnable(GL_PROGRAM_POINT_SIZE);

    // 4. Disable MSAA
    glDisable(GL_MULTISAMPLE);

    // Log Limits
    int maxBlockSize;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxBlockSize);

    
    LOG_INFO("Renderer Initialized.");
    LOG_INFO("  Debug Mode: %s", debugEnabled ? "Enabled" : "Disabled");
    LOG_INFO("  Max Compute Invocations: %d", maxBlockSize);
}

void Renderer::Shutdown() {
    // Cleanup resources if necessary
}

void Renderer::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount) {
    vertexArray->Bind();
    uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
    
    s_Stats.DrawCalls++;
    s_Stats.IndexCount += count;
}

void Renderer::DrawMultiIndirect(const std::shared_ptr<VertexArray>& vertexArray, 
                                 uint32_t drawCount, uint32_t stride) {
    vertexArray->Bind();
    // Assuming GL_POINTS for your particle system
    glMultiDrawArraysIndirect(GL_POINTS, (void*)0, drawCount, stride);

    s_Stats.DrawCalls++;
    s_Stats.IndirectDrawCount += drawCount;
}




