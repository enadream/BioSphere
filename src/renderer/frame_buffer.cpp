#include "renderer/frame_buffer.hpp"
#include "core/log.hpp"

#include <glad/glad.h>

FrameBuffer::FrameBuffer(const FrameBufferSpecification& spec) : m_Specification(spec){
    Invalidate();
}

FrameBuffer::~FrameBuffer() {
    glDeleteFramebuffers(1, &m_RendererID);
    glDeleteTextures(1, &m_ColorAttachment);
    glDeleteTextures(1, &m_DepthAttachment);
}

FrameBuffer::FrameBuffer(FrameBuffer&& other) noexcept
    : m_RendererID(other.m_RendererID), 
      m_ColorAttachment(other.m_ColorAttachment), 
      m_DepthAttachment(other.m_DepthAttachment), 
      m_Specification(other.m_Specification)
{
    other.m_RendererID = 0;
    other.m_ColorAttachment = 0;
    other.m_DepthAttachment = 0;
}

void FrameBuffer::Invalidate(){
    if (m_RendererID){
        glDeleteFramebuffers(1, &m_RendererID);
        glDeleteTextures(1, &m_ColorAttachment);
        glDeleteTextures(1, &m_DepthAttachment);
    }

    glCreateFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

    // 1. Color Attachment
    if (m_Specification.ColorFormat != FrameBufferTextureFormat::None) {
        glCreateTextures(GL_TEXTURE_2D, 1, &m_ColorAttachment);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
        
        // Setup for basic color storage (RGBA8)
        glTextureStorage2D(m_ColorAttachment, 1, GL_RGBA8, m_Specification.Width, m_Specification.Height);
        
        // Filtering
        glTextureParameteri(m_ColorAttachment, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_ColorAttachment, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_ColorAttachment, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_ColorAttachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_ColorAttachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glNamedFramebufferTexture(m_RendererID, GL_COLOR_ATTACHMENT0, m_ColorAttachment, 0);
    }

    // 2. Depth Attachment
    if (m_Specification.DepthFormat != FrameBufferTextureFormat::None) {
        glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthAttachment);
        glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
        
        // Use 24 bit depth + 8 bit stencil
        glTextureStorage2D(m_DepthAttachment, 1, GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height);
        
        glNamedFramebufferTexture(m_RendererID, GL_DEPTH_STENCIL_ATTACHMENT, m_DepthAttachment, 0);
    }

    if (glCheckNamedFramebufferStatus(m_RendererID, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_FATAL("Framebuffer is incomplete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0 || width > 8192 || height > 8192) {
        LOG_WARN("Attempted to resize framebuffer to %d, %d", width, height);
        return;
    }

    m_Specification.Width = width;
    m_Specification.Height = height;
    
    Invalidate();
}