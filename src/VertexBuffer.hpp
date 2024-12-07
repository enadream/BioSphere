#ifndef VERTEX_BUFFER_HPP
#define VERTEX_BUFFER_HPP

#include <GL/glew.h>

class VertexBuffer {
private:
    uint32_t ID;
public:
    VertexBuffer(uint32_t size, void* data, GLenum usage);
    ~VertexBuffer();

    void Bind();
};


#endif