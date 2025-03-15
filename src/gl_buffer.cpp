#include <stdio.h>

#include "gl_buffer.hpp"


Buffer::Buffer(GLenum type) : m_BOID(0), m_Amount(0), m_Size(0), m_Type(type), p_PeristentBuffer(nullptr) {}

Buffer::Buffer(GLenum type, uint32_t size, const void*data, uint32_t amount, GLenum usage) : m_Type(type){
    glGenBuffers(1, &m_BOID);
    glBindBuffer(type, m_BOID);
    glBufferData(type, size, data, usage);
    m_Amount = amount;
    m_Size = size;
}

Buffer::~Buffer() {
    glDeleteBuffers(1, &m_BOID);
}

Buffer::Buffer(Buffer && other) noexcept : m_BOID(other.m_BOID), m_Amount(other.m_Amount), m_Size(other.m_Size),
    m_Type(other.m_Type), p_PeristentBuffer(other.p_PeristentBuffer)  {
    other.m_BOID = 0;
}

void Buffer::GenBuffer(uint32_t size, const void*data, uint32_t amount, GLenum usage){
    if (m_BOID == 0){
        glGenBuffers(1, &m_BOID);
        glBindBuffer(m_Type, m_BOID);
        glBufferData(m_Type, size, data, usage);
        m_Amount = amount;
        m_Size = size;
        // unbind
        glBindBuffer(m_Type, 0);
    }
    else {
        printf("[ERROR]: Buffer couldn't generated because buffer already generated!.\n");
    }
}

void Buffer::GenBufferStorage(uint32_t size, const void*data, uint32_t amount, GLbitfield flags){
    if (m_BOID == 0){
        glGenBuffers(1, &m_BOID);
        glBindBuffer(m_Type, m_BOID);
        glBufferStorage(m_Type, size, data, flags);
        m_Amount = amount;
        m_Size = size;
        // unbind
        glBindBuffer(m_Type, 0);
    }
    else {
        printf("[ERROR]: Buffer couldn't generated because buffer already generated!.\n");
    }
}

void Buffer::GenPersistentBuffer(uint32_t offset, uint32_t size, GLbitfield flags){
    Bind();
    // unmap old map
    if (!p_PeristentBuffer){
        glUnmapBuffer(m_Type);
    }
    p_PeristentBuffer = glMapBufferRange(m_Type, offset, size, flags);

    if (!p_PeristentBuffer){
        printf("[ERROR]: Failed to map persistent buffer.\n");
    }
    Unbind();
}
