#pragma once

#include "renderer/vertex_buffer.hpp"
#include "renderer/index_buffer.hpp"
#include <memory>
#include <vector>

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    // RAII - No copying, only moving
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    void Bind() const { glBindVertexArray(m_RendererID); }
    void Unbind() const { glBindVertexArray(0); }

    // Decoupled: We pass a pointer to an existing buffer.
    // The VAO reads the layout from the VBO and sets up attributes automatically.
    void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer);
    
    // Decoupled: We pass a pointer to an existing IBO.
    void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);

    const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const { return m_VertexBuffers; }
    const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }
    
    uint32_t GetRendererID() const { return m_RendererID; }

private:
    uint32_t m_RendererID;
    uint32_t m_VertexBufferIndex = 0; // Tracks the binding point index (binding=0, binding=1...)
    uint32_t m_AttributeIndex = 0;    // Tracks the shader location index (layout(location=0)...)
    
    // Aggregation: We hold references, we don't own the memory of the buffer data itself
    std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
    std::shared_ptr<IndexBuffer> m_IndexBuffer;
};