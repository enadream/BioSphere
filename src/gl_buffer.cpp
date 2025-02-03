#include <stdio.h>

#include "gl_buffer.hpp"


Buffer::Buffer(GLenum type) : m_Type(type), m_BOID(0), m_Amount(0) {}

Buffer::Buffer(GLenum type, uint32_t size, const void*data, uint32_t amount, GLenum usage) : m_Type(type){
    glGenBuffers(1, &m_BOID);
    glBindBuffer(type, m_BOID);
    glBufferData(type, size, data, usage);
    m_Amount = amount;
}

Buffer::~Buffer() {
    glDeleteBuffers(1, &m_BOID);
}

Buffer::Buffer(Buffer && other) noexcept : m_BOID (other.m_BOID), m_Type(other.m_Type), m_Amount(other.m_Amount) {
    other.m_BOID = 0;
}

void Buffer::GenBuffer(uint32_t size, const void*data, uint32_t amount, GLenum usage){
    if (m_BOID == 0){
        glGenBuffers(1, &m_BOID);
        glBindBuffer(m_Type, m_BOID);
        glBufferData(m_Type, size, data, usage);
        m_Amount = amount;
        // unbind
        glBindBuffer(m_Type, 0);
    }
    else {
        printf("[ERROR]: Buffer couldn't generated because buffer already generated!.\n");
    }
}