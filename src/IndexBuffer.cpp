#include "IndexBuffer.hpp"

IndexBuffer::IndexBuffer(uint32_t size, void* data, GLenum usage){
    glGenBuffers(1, &ID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
}

IndexBuffer::~IndexBuffer(){
    glDeleteBuffers(1, &ID);
}

void IndexBuffer::Bind(){
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
}