#include "frame_buffer.hpp"


FrameBuffer::FrameBuffer(GLenum type) : m_Type(type){
    glGenFramebuffers(1, &m_BOID);
}

FrameBuffer::~FrameBuffer() {
    glDeleteFramebuffers(1, &m_BOID);
}

FrameBuffer::FrameBuffer(FrameBuffer && other) noexcept : m_Type(other.m_Type), m_BOID(other.m_BOID) {
    other.m_BOID = 0;
}