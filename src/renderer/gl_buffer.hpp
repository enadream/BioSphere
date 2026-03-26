#pragma once

#include <glad/glad.h>
#include <cstdint>

// A generic OpenGL Buffer wrapper.
// Useful for:
// - Shader Storage Buffers (SSBO)
// - Uniform Buffers (UBO)
// - Atomic Counter Buffers
// - Indirect Draw Buffers
class GLBuffer {
public:
    // Default constructor (empty)
    GLBuffer() = default;

    // Create a generic buffer
    // usage: GL_DYNAMIC_DRAW, GL_STATIC_DRAW, etc.
    GLBuffer(uint32_t size, const void* data = nullptr, GLenum usage = GL_DYNAMIC_DRAW);

    // Create an immutable storage buffer (for persistent mapping)
    // flags: GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
    GLBuffer(uint32_t size, GLbitfield flags, const void* data = nullptr);

    ~GLBuffer();

    // RAII
    GLBuffer(const GLBuffer&) = delete;
    GLBuffer& operator=(const GLBuffer&) = delete;
    GLBuffer(GLBuffer&&) noexcept;
    GLBuffer& operator=(GLBuffer&&) noexcept;

    // --- Creation ---
    void Create(uint32_t size, const void* data = nullptr, GLenum usage = GL_DYNAMIC_DRAW);
    void CreateStorage(uint32_t size, GLbitfield flags, const void* data = nullptr);

    // --- Updates ---
    void SetData(const void* data, uint32_t size, uint32_t offset = 0);

    // --- Binding (Indexed) ---
    // Binds the buffer to a specific indexed target (e.g., binding=0 in shader)
    // target: GL_SHADER_STORAGE_BUFFER, GL_UNIFORM_BUFFER, GL_ATOMIC_COUNTER_BUFFER
    void BindBase(GLenum target, uint32_t index) const { glBindBufferBase(target, index, m_RendererID); }

    // --- Binding (Generic) ---
    // target: GL_DRAW_INDIRECT_BUFFER, GL_DISPATCH_INDIRECT_BUFFER
    void Bind(GLenum target) const { glBindBuffer(target, m_RendererID); }
    void Unbind(GLenum target) const { glBindBuffer(target, 0); }

    // --- Mapping ---
    void* MapRange(uint32_t offset, uint32_t length, GLbitfield access);
    void* MapPersistent(GLbitfield access);
    void Unmap();

    // --- Getters ---
    uint32_t GetRendererID() const { return m_RendererID; }
    uint32_t GetSize() const { return m_Size; }

private:
    uint32_t m_RendererID = 0;
    uint32_t m_Size = 0;
    void* m_MappedPointer = nullptr;
    bool m_IsStorage = false;
};