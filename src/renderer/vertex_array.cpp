#include "renderer/vertex_array.hpp"
#include "core/log.hpp"
#include <glad/glad.h>

static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:    return GL_FLOAT;
        case ShaderDataType::Float2:   return GL_FLOAT;
        case ShaderDataType::Float3:   return GL_FLOAT;
        case ShaderDataType::Float4:   return GL_FLOAT;
        case ShaderDataType::Mat3:     return GL_FLOAT;
        case ShaderDataType::Mat4:     return GL_FLOAT;
        case ShaderDataType::Int:      return GL_INT;
        case ShaderDataType::Int2:     return GL_INT;
        case ShaderDataType::Int3:     return GL_INT;
        case ShaderDataType::Int4:     return GL_INT;
        case ShaderDataType::Bool:     return GL_BOOL;
        default: return 0;
    }
}

VertexArray::VertexArray() {
    glCreateVertexArrays(1, &m_RendererID);
}

VertexArray::~VertexArray() {
    glDeleteVertexArrays(1, &m_RendererID);
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_RendererID(other.m_RendererID), 
      m_VertexBufferIndex(other.m_VertexBufferIndex),
      m_AttributeIndex(other.m_AttributeIndex),
      m_VertexBuffers(std::move(other.m_VertexBuffers)),
      m_IndexBuffer(std::move(other.m_IndexBuffer))
{
    other.m_RendererID = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        glDeleteVertexArrays(1, &m_RendererID);
        m_RendererID = other.m_RendererID;
        m_VertexBufferIndex = other.m_VertexBufferIndex;
        m_AttributeIndex = other.m_AttributeIndex;
        m_VertexBuffers = std::move(other.m_VertexBuffers);
        m_IndexBuffer = std::move(other.m_IndexBuffer);
        
        other.m_RendererID = 0;
    }
    return *this;
}

void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer) {
    if (vertexBuffer->GetLayout().GetElements().empty()) {
        LOG_ERROR("Vertex Array: Tried to add a VBO with no layout defined!");
        return;
    }

    // DSA: Bind the VBO to a specific binding point (m_VertexBufferIndex)
    glVertexArrayVertexBuffer(m_RendererID, m_VertexBufferIndex, vertexBuffer->GetRendererID(), 0, vertexBuffer->GetLayout().GetStride());

    const auto& layout = vertexBuffer->GetLayout();
    for (const auto& element : layout) {
        // 1. Enable the attribute (e.g., location = 0)
        glEnableVertexArrayAttrib(m_RendererID, m_AttributeIndex);
        
        // 2. Link the attribute index to the binding point
        glVertexArrayAttribBinding(m_RendererID, m_AttributeIndex, m_VertexBufferIndex);

        GLenum type = ShaderDataTypeToOpenGLBaseType(element.Type);
        
        // 3. Define the format (Float vs Int requires different functions)
        if (type == GL_INT || type == GL_UNSIGNED_INT || type == GL_BYTE || type == GL_UNSIGNED_BYTE) {
             glVertexArrayAttribIFormat(m_RendererID, m_AttributeIndex, element.GetComponentCount(), type, element.Offset);
        } else {
             glVertexArrayAttribFormat(m_RendererID, m_AttributeIndex, element.GetComponentCount(), type, element.Normalized, element.Offset);
        }

        // Move to next attribute location (e.g., pos=0 -> normal=1)
        m_AttributeIndex++;
    }

    m_VertexBuffers.push_back(vertexBuffer);
    // Move to next binding point for the next VBO
    m_VertexBufferIndex++; 
}

void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) {
    // DSA: Attach IBO to VAO
    glVertexArrayElementBuffer(m_RendererID, indexBuffer->GetRendererID());
    m_IndexBuffer = indexBuffer;
}