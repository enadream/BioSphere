#pragma once

#include <cstdint>
#include <glad/glad.h>
#include <glm/glm.hpp>

enum class FrameBufferTextureFormat : uint32_t {
    None = 0,
    // Color
    RGBA8,
    // Depth/Stencil
    DEPTH24STENCIL8,
    // Defaults
    Depth = DEPTH24STENCIL8
};

struct FrameBufferSpecification {
    uint32_t Width = 0, Height = 0;
    FrameBufferTextureFormat ColorFormat = FrameBufferTextureFormat::RGBA8;
    FrameBufferTextureFormat DepthFormat = FrameBufferTextureFormat::Depth;
    uint32_t Samples = 1; // 1 = no MSAA
    bool SwapChainTarget = false; // Is this the screen (0)
};

class FrameBuffer {
public: // functions
    // Create FBO with specific config
    FrameBuffer(const FrameBufferSpecification& spec);
    ~FrameBuffer();

    // Prevent copying (RAII)
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;
    // Allow moving
    FrameBuffer(FrameBuffer&& other) noexcept;

    void Invalidate(); // Re-creates the GPU resources (Call on Init or Resize)
    void Resize(uint32_t width, uint32_t height);

    inline void Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
        glViewport(0, 0, m_Specification.Width, m_Specification.Height);
    }
    inline void Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // --- Accessors ---
    uint32_t GetColorAttachmentRendererID() const { return m_ColorAttachment; }
    uint32_t GetDepthAttachmentRendererID() const { return m_DepthAttachment; }
    
    const FrameBufferSpecification& GetSpecification() const { return m_Specification; }
    uint32_t GetRendererID() const { return m_RendererID; }

private: // variables
    uint32_t m_RendererID = 0;
    uint32_t m_ColorAttachment = 0;
    uint32_t m_DepthAttachment = 0;
    FrameBufferSpecification m_Specification;
};
