#include "VertexBuffer.hpp"

VertexBuffer::VertexBuffer(uint32_t size, void* data, GLenum usage){
    glGenBuffers(1, &ID);
    glBindBuffer(GL_ARRAY_BUFFER, ID);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
}

VertexBuffer::~VertexBuffer(){
    glDeleteBuffers(1, &ID);
}

void VertexBuffer::Bind(){
    glBindBuffer(GL_ARRAY_BUFFER, ID);
}