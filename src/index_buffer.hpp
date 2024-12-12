#ifndef INDEX_BUFFER_HPP
#define INDEX_BUFFER_HPP

//#include <GL/glew.h>

class IndexBuffer {
private:
    uint32_t ID;
public:
    IndexBuffer(uint32_t size, void* data, GLenum usage);
    ~IndexBuffer();

    void Bind();
};


#endif