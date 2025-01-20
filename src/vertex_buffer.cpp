#include <stdio.h>
#include "vertex_buffer.hpp"

VertexBuffer::VertexBuffer() : m_VBO(0), m_VertAmount(0) {}

VertexBuffer::VertexBuffer(uint32_t size, const void*data, uint32_t vert_amount, GLenum usage) {
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    m_VertAmount = vert_amount;
}

VertexBuffer::~VertexBuffer() {
    glDeleteBuffers(1, &m_VBO);
}

VertexBuffer::VertexBuffer(VertexBuffer && other) noexcept : m_VBO(other.m_VBO) {
    other.m_VBO = 0;
}

void VertexBuffer::GenVertexBuffer(uint32_t size, const void*data, uint32_t vert_amount, GLenum usage){
    if (m_VBO == 0){
        glGenBuffers(1, &m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, size, data, usage);
        m_VertAmount = vert_amount;
    }
    else {
        printf("[ERROR]: Vertex buffer couldn't generated because Vertex buffer already generated!.\n");
    }
}