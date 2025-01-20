#ifndef INDEX_BUFFER_HPP
#define INDEX_BUFFER_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>

enum class IndexType : uint32_t {
    UNDEFINED = 0,
    UINT8 = GL_UNSIGNED_BYTE,
    UINT16 = GL_UNSIGNED_SHORT,
    UINT32 = GL_UNSIGNED_INT,
};

class IndexBuffer {
public: // functions
    IndexBuffer(uint32_t size, const void*data, IndexType data_type, GLenum usage = GL_STATIC_DRAW);
    IndexBuffer();
    ~IndexBuffer();

    // delete copy constructors
    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;    
    // move constructor
    IndexBuffer(IndexBuffer&& other) noexcept;

    void GenIndexBuffer(uint32_t size, const void*data, IndexType data_type, GLenum usage = GL_STATIC_DRAW);

    inline void Bind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    }
    inline uint32_t Count() const {
        return m_Count;
    }
    inline uint32_t GetID() const {
        return m_EBO;
    }
    inline uint32_t GetType() const {
        return static_cast<uint32_t>(m_DataType);
    }
private: // variables
    uint32_t m_EBO;
    IndexType m_DataType;
    uint32_t m_Count;
};

#endif