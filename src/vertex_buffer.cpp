#include <stdio.h>
#include "vertex_buffer.hpp"

VertexBuffer::VertexBuffer() : m_VBO(0), m_VertAmount(0), m_Size(0), p_PeristentBuffer(nullptr) {}

VertexBuffer::VertexBuffer(uint32_t size, const void*data, uint32_t vert_amount, GLenum usage) {
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    m_VertAmount = vert_amount;
    m_Size = size;
}

VertexBuffer::~VertexBuffer() {
    glDeleteBuffers(1, &m_VBO);
}

VertexBuffer::VertexBuffer(VertexBuffer && other) noexcept : m_VBO(other.m_VBO), m_VertAmount(other.m_VertAmount), m_Size(other.m_Size),
    p_PeristentBuffer(other.p_PeristentBuffer){
    other.m_VBO = 0;
}

void VertexBuffer::GenPersistentBuffer(uint32_t offset, uint32_t size, GLbitfield flags){
    Bind();
    // unmap old map
    if (!p_PeristentBuffer){
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    p_PeristentBuffer = glMapBufferRange(GL_ARRAY_BUFFER, offset, size, flags);

    if (!p_PeristentBuffer){
        printf("[ERROR]: Failed to map persistent buffer.\n");
    }
    Unbind();
}

void VertexBuffer::GenVertexBuffer(uint32_t size, const void*data, uint32_t vert_amount, GLenum usage){
    if (m_VBO == 0){
        glGenBuffers(1, &m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, size, data, usage);
        m_VertAmount = vert_amount;
        m_Size = size;
        // unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else {
        printf("[ERROR]: Vertex buffer couldn't generated because Vertex buffer already generated!.\n");
    }
}

void VertexBuffer::GenVertexBufferStorage(uint32_t size, const void*data, uint32_t vert_amount, GLbitfield flags){
    if (m_VBO == 0){
        glGenBuffers(1, &m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferStorage(GL_ARRAY_BUFFER, size, data, flags);
        m_VertAmount = vert_amount;
        m_Size = size;
        // unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else {
        printf("[ERROR]: Vertex buffer couldn't generated because Vertex buffer already generated!.\n");
    }
}