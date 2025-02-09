#ifndef FRAME_BUFFER_HPP
#define FRAME_BUFFER_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class FrameBuffer {
public: // functions
    FrameBuffer(GLenum type);
    ~FrameBuffer();

    // delete copy constructors
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;
    // move constructor
    FrameBuffer(FrameBuffer && other) noexcept;

    inline void Bind() const {
        glBindFramebuffer(m_Type, m_BOID);
    }
    inline void Unbind() const {
        glBindFramebuffer(m_Type, 0);
    }
    inline uint32_t GetID() const{
        return m_BOID;
    }
    inline GLenum GetType() const {
        return m_Type;
    }
private: // variables
    uint32_t m_BOID; // buffer object ID
    GLenum m_Type;
};

#endif