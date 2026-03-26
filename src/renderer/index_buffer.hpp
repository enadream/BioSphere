#pragma once

#include <cstdint>
#include <glad/glad.h>

class IndexBuffer {
public:
    // Creates and uploads data immediately
    IndexBuffer(uint32_t* indices, uint32_t count);
    ~IndexBuffer();

    // RAII
    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;

    void Bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID); }
    void Unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

    uint32_t GetCount() const { return m_Count; }
    uint32_t GetRendererID() const { return m_RendererID; }

private:
    uint32_t m_RendererID;
    uint32_t m_Count;
};