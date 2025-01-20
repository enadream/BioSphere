#include <stdio.h>
#include <utility>
#include "vertex_array.hpp"

VertexArray::VertexArray(){
    glGenVertexArrays(1, &m_VAO);
}

VertexArray::~VertexArray(){
    glDeleteVertexArrays(1, &m_VAO);
}

VertexArray::VertexArray(VertexArray && other) noexcept :
    m_VertBuffer(std::move(other.m_VertBuffer)), m_IndxBuffer(std::move(other.m_IndxBuffer)) {
    m_VAO = other.m_VAO;
    other.m_VAO = 0;
}

void VertexArray::GenVertexBuffer(uint32_t size, const void*data, uint32_t vert_amount, GLenum usage){
    Bind();
    m_VertBuffer.GenVertexBuffer(size, data, vert_amount, usage);
}

void VertexArray::GenIndexBuffer(uint32_t size, const void*data, IndexType data_type, GLenum usage){
    Bind();
    m_IndxBuffer.GenIndexBuffer(size, data, data_type, usage);
}

void VertexArray::InsertLayout(uint32_t index, uint32_t count, GLenum gl_type, GLboolean normalize, uint32_t row_size, void* offset){
    if (m_VertBuffer.GetID()){
        Bind();
        m_VertBuffer.Bind();
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, count, gl_type, normalize, row_size, offset);
    }
    else {
        printf("[ERROR]: You can't insert layout because Vertex buffer didn't generated.\n");
    }
}