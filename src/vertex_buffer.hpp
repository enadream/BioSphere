#ifndef VERTEX_BUFFER_HPP
#define VERTEX_BUFFER_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class VertexBuffer {
public: // functions
    VertexBuffer();
    VertexBuffer(uint32_t size, const void*data, uint32_t vert_amount, GLenum usage = GL_STATIC_DRAW);
    ~VertexBuffer();

    // delete copy constructors
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    // move constructor
    VertexBuffer(VertexBuffer && other) noexcept;
    
    void GenVertexBuffer(uint32_t size, const void*data, uint32_t vert_amount, GLenum usage = GL_STATIC_DRAW);
    void GenVertexBufferStorage(uint32_t size, const void*data, uint32_t vert_amount, GLbitfield flags);
    void GenPersistentBuffer(uint32_t offset, uint32_t size, GLbitfield flags);
    
    inline void* GetMap() const {
        return p_PeristentBuffer;
    }
    inline void Bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    }
    inline void Unbind() const {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    inline uint32_t GetID() const{
        return m_VBO;
    }
    inline uint32_t GetVertAmount() const {
        return m_VertAmount;
    }
    inline uint32_t GetSize() const {
        return m_Size;
    }
    
private: // variables
    uint32_t m_VBO;
    uint32_t m_VertAmount;
    uint32_t m_Size; // buffer size in bytes
    void *p_PeristentBuffer;
};

#endif