#ifndef VERTEX_ARRAY_HPP
#define VERTEX_ARRAY_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "vertex_buffer.hpp"
#include "index_buffer.hpp"


class VertexArray {
public: // functions
    VertexArray();
    ~VertexArray();

    // delete copy constructors
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    // move constructor
    VertexArray(VertexArray && other) noexcept;

    void GenVertexBuffer(uint32_t size, const void*data, uint32_t vert_amount, GLenum usage = GL_STATIC_DRAW);
    void GenIndexBuffer(uint32_t size, const void*data, IndexType data_type, GLenum usage = GL_STATIC_DRAW);
    void InsertLayout(uint32_t index, uint32_t count, GLenum gl_type, GLboolean normalize, uint32_t stride, uint32_t offset);
    void InsertLayoutI(uint32_t index, uint32_t count, GLenum gl_type, uint32_t stride, uint32_t offset);
    
    inline void Bind() const {
        glBindVertexArray(m_VAO);
    }
    inline void Unbind() const {
        glBindVertexArray(0);
    }
public:
    VertexBuffer m_VertBuffer;
    IndexBuffer m_IndxBuffer;

private: // variables
    uint32_t m_VAO;
};

#endif