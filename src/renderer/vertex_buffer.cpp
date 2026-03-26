#include "renderer/vertex_buffer.hpp"
#include "core/log.hpp"

VertexBuffer::VertexBuffer() : m_RendererID(0), m_Size(0) {}

// Standard Constructor (Mutable)
VertexBuffer::VertexBuffer(uint32_t size, const void* data, GLenum usage) {
    Create(size, data, usage);
}

// Storage Constructor (Immutable)
VertexBuffer::VertexBuffer(uint32_t size, GLbitfield flags, const void* data) {
    CreateStorage(size, flags, data);
}

VertexBuffer::~VertexBuffer() {
    if (m_RendererID) {
        // If mapped, unmap before deleting (good practice, though driver usually handles it)
        if (m_MappedPointer) {
            glUnmapNamedBuffer(m_RendererID);
        }
        glDeleteBuffers(1, &m_RendererID);
    }
}

// Move Constructor
VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept 
    : m_RendererID(other.m_RendererID), m_Size(other.m_Size), 
      m_MappedPointer(other.m_MappedPointer), m_Layout(std::move(other.m_Layout)),
      m_IsStorage(other.m_IsStorage)
{
    other.m_RendererID = 0;
    other.m_Size = 0;
    other.m_MappedPointer = nullptr;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
    if (this != &other) {
        if (m_RendererID) glDeleteBuffers(1, &m_RendererID);
        
        m_RendererID = other.m_RendererID;
        m_Size = other.m_Size;
        m_MappedPointer = other.m_MappedPointer;
        m_Layout = std::move(other.m_Layout);
        m_IsStorage = other.m_IsStorage;

        other.m_RendererID = 0;
        other.m_Size = 0;
        other.m_MappedPointer = nullptr;
    }
    return *this;
}

void VertexBuffer::Create(uint32_t size, const void* data, GLenum usage) {
    if (m_RendererID) glDeleteBuffers(1, &m_RendererID);

    glCreateBuffers(1, &m_RendererID);
    glNamedBufferData(m_RendererID, size, data, usage);
    m_Size = size;
    m_IsStorage = false;
}

void VertexBuffer::CreateStorage(uint32_t size, GLbitfield flags, const void* data) {
    if (m_RendererID) glDeleteBuffers(1, &m_RendererID);

    glCreateBuffers(1, &m_RendererID);
    glNamedBufferStorage(m_RendererID, size, data, flags);
    m_Size = size;
    m_IsStorage = true;
}

void VertexBuffer::SetData(const void* data, uint32_t size, uint32_t offset) {
    if (size + offset > m_Size) {
        LOG_ERROR("VertexBuffer Overflow: Attempted to write %d bytes at offset %d, but buffer size is %d", size, offset, m_Size);
        return;
    }
    glNamedBufferSubData(m_RendererID, offset, size, data);
}

void* VertexBuffer::MapRange(uint32_t offset, uint32_t length, GLbitfield access) {
    if (!m_RendererID) return nullptr;
    return glMapNamedBufferRange(m_RendererID, offset, length, access);
}

void* VertexBuffer::MapPersistent(GLbitfield access) {
    if (!m_RendererID) return nullptr;
    
    // Safety check: Persistent mapping usually requires the buffer to be created with GL_MAP_PERSISTENT_BIT
    if (!m_IsStorage) {
        LOG_WARN("Attempting persistent map on a buffer not created with CreateStorage/glBufferStorage.");
    }

    // Map the whole buffer
    m_MappedPointer = glMapNamedBufferRange(m_RendererID, 0, m_Size, access);
    return m_MappedPointer;
}

void VertexBuffer::Unmap() {
    if (m_RendererID) {
        glUnmapNamedBuffer(m_RendererID);
        m_MappedPointer = nullptr;
    }
}