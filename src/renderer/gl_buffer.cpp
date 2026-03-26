#include "renderer/gl_buffer.hpp"
#include "core/log.hpp"

GLBuffer::GLBuffer(uint32_t size, const void* data, GLenum usage) {
    Create(size, data, usage);
}

GLBuffer::GLBuffer(uint32_t size, GLbitfield flags, const void* data) {
    CreateStorage(size, flags, data);
}

GLBuffer::~GLBuffer() {
    if (m_RendererID) {
        if (m_MappedPointer) {
            glUnmapNamedBuffer(m_RendererID);
        }
        glDeleteBuffers(1, &m_RendererID);
    }
}

GLBuffer::GLBuffer(GLBuffer&& other) noexcept
    : m_RendererID(other.m_RendererID), 
      m_Size(other.m_Size), 
      m_MappedPointer(other.m_MappedPointer), 
      m_IsStorage(other.m_IsStorage)
{
    other.m_RendererID = 0;
    other.m_Size = 0;
    other.m_MappedPointer = nullptr;
}

GLBuffer& GLBuffer::operator=(GLBuffer&& other) noexcept {
    if (this != &other) {
        if (m_RendererID) glDeleteBuffers(1, &m_RendererID);
        
        m_RendererID = other.m_RendererID;
        m_Size = other.m_Size;
        m_MappedPointer = other.m_MappedPointer;
        m_IsStorage = other.m_IsStorage;

        other.m_RendererID = 0;
        other.m_Size = 0;
        other.m_MappedPointer = nullptr;
    }
    return *this;
}

void GLBuffer::Create(uint32_t size, const void* data, GLenum usage) {
    if (m_RendererID) glDeleteBuffers(1, &m_RendererID);
    
    glCreateBuffers(1, &m_RendererID);
    glNamedBufferData(m_RendererID, size, data, usage);
    m_Size = size;
    m_IsStorage = false;
}

void GLBuffer::CreateStorage(uint32_t size, GLbitfield flags, const void* data) {
    if (m_RendererID) glDeleteBuffers(1, &m_RendererID);

    glCreateBuffers(1, &m_RendererID);
    glNamedBufferStorage(m_RendererID, size, data, flags);
    m_Size = size;
    m_IsStorage = true;
}

void GLBuffer::SetData(const void* data, uint32_t size, uint32_t offset) {
    if (size + offset > m_Size) {
        LOG_ERROR("GLBuffer Overflow: Attempted to write %d bytes at offset %d, but buffer size is %d", size, offset, m_Size);
        return;
    }
    glNamedBufferSubData(m_RendererID, offset, size, data);
}

void* GLBuffer::MapRange(uint32_t offset, uint32_t length, GLbitfield access) {
    if (!m_RendererID) return nullptr;
    return glMapNamedBufferRange(m_RendererID, offset, length, access);
}

void* GLBuffer::MapPersistent(GLbitfield access) {
    if (!m_RendererID) return nullptr;
    if (!m_IsStorage) {
        LOG_WARN("Attempting persistent map on a buffer not created with immutable storage!");
    }
    m_MappedPointer = glMapNamedBufferRange(m_RendererID, 0, m_Size, access);
    return m_MappedPointer;
}

void GLBuffer::Unmap() {
    if (m_RendererID) {
        glUnmapNamedBuffer(m_RendererID);
        m_MappedPointer = nullptr;
    }
}