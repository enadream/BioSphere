#ifndef GPU_BUFFER_HPP
#define GPU_BUFFER_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Buffer {
public: // functions
    Buffer(GLenum type);
    Buffer(GLenum type, uint32_t size, const void*data, uint32_t amount, GLenum usage = GL_STATIC_DRAW);
    ~Buffer();

    // delete copy constructors
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    // move constructor
    Buffer(Buffer && other) noexcept;
    
    void GenBuffer(uint32_t size, const void*data, uint32_t amount, GLenum usage = GL_STATIC_DRAW);
    void GenBufferStorage(uint32_t size, const void*data, uint32_t amount, GLbitfield flags);
    void GenPersistentBuffer(uint32_t offset, uint32_t size, GLbitfield flags);

    inline void* GetMap() const {
        return p_PeristentBuffer;
    }
    inline void Bind() const {
        glBindBuffer(m_Type, m_BOID);
    }
    inline void Unbind() const {
        glBindBuffer(m_Type, 0);
    }
    inline uint32_t GetID() const {
        return m_BOID;
    }
    inline uint32_t GetAmount() const {
        return m_Amount;
    }
    inline uint32_t GetSize() const {
        return m_Size;
    }
    inline GLenum GetType() const {
        return m_Type;
    }

private: // variables
    uint32_t m_BOID; // buffer object ID
    uint32_t m_Amount; // amount of the buffer object
    uint32_t m_Size; // size of the buffer object in bytes
    GLenum m_Type;
    void *p_PeristentBuffer;
};


#endif