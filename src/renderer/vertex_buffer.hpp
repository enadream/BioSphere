#pragma once

#include <glad/glad.h>
#include <vector>
#include <string>
#include <cstdint>

// --- Buffer Layout System (Describes the data structure) ---

enum class ShaderDataType {
    None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
};

static uint32_t ShaderDataTypeSize(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:    return 4;
        case ShaderDataType::Float2:   return 4 * 2;
        case ShaderDataType::Float3:   return 4 * 3;
        case ShaderDataType::Float4:   return 4 * 4;
        case ShaderDataType::Mat3:     return 4 * 3 * 3;
        case ShaderDataType::Mat4:     return 4 * 4 * 4;
        case ShaderDataType::Int:      return 4;
        case ShaderDataType::Int2:     return 4 * 2;
        case ShaderDataType::Int3:     return 4 * 3;
        case ShaderDataType::Int4:     return 4 * 4;
        case ShaderDataType::Bool:     return 1;
        default: return 0;
    }
}

struct BufferElement {
    std::string Name;
    ShaderDataType Type;
    uint32_t Size;
    uint32_t Offset;
    bool Normalized;

    BufferElement() = default;
    BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
        : Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized) {}

    uint32_t GetComponentCount() const {
        switch (Type) {
            case ShaderDataType::Float:   return 1;
            case ShaderDataType::Float2:  return 2;
            case ShaderDataType::Float3:  return 3;
            case ShaderDataType::Float4:  return 4;
            case ShaderDataType::Mat3:    return 3 * 3;
            case ShaderDataType::Mat4:    return 4 * 4;
            case ShaderDataType::Int:     return 1;
            case ShaderDataType::Int2:    return 2;
            case ShaderDataType::Int3:    return 3;
            case ShaderDataType::Int4:    return 4;
            case ShaderDataType::Bool:    return 1;
            default: return 0;
        }
    }
};

class BufferLayout {
public:
    BufferLayout() {}
    BufferLayout(const std::initializer_list<BufferElement>& elements) 
        : m_Elements(elements) 
    {
        CalculateOffsetsAndStride();
    }

    inline uint32_t GetStride() const { return m_Stride; }
    inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }

    std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
    std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
    std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
    std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }

private:
    void CalculateOffsetsAndStride() {
        uint32_t offset = 0;
        m_Stride = 0;
        for (auto& element : m_Elements) {
            element.Offset = offset;
            offset += element.Size;
            m_Stride += element.Size;
        }
    }

    std::vector<BufferElement> m_Elements;
    uint32_t m_Stride = 0;
};

// --- Vertex Buffer Class ---

class VertexBuffer {
public:
    // Default constructor (creates empty object, use Create/Allocate later)
    VertexBuffer();
    
    // Standard Mutable Buffer (glBufferData)
    // Usage: GL_STATIC_DRAW, GL_DYNAMIC_DRAW
    VertexBuffer(uint32_t size, const void* data = nullptr, GLenum usage = GL_DYNAMIC_DRAW);
    
    // Immutable Storage Buffer (glBufferStorage)
    // Flags: GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
    VertexBuffer(uint32_t size, GLbitfield flags, const void* data = nullptr);

    ~VertexBuffer();

    // Move Semantics
    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    // --- Initialization ---
    // Replaces "GenVertexBuffer". Creates standard mutable buffer.
    void Create(uint32_t size, const void* data = nullptr, GLenum usage = GL_DYNAMIC_DRAW);
    
    // Replaces "GenVertexBufferStorage". Creates immutable storage.
    void CreateStorage(uint32_t size, GLbitfield flags, const void* data = nullptr);

    // --- Data Manipulation ---
    void SetData(const void* data, uint32_t size, uint32_t offset = 0);
    
    // --- Mapping (DSA) ---
    void* MapRange(uint32_t offset, uint32_t length, GLbitfield access);
    void* MapPersistent(GLbitfield access); // Maps entire buffer
    void Unmap();

    // --- Bindings ---
    void Bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_RendererID); }
    void Unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

    // --- Getters ---
    uint32_t GetRendererID() const { return m_RendererID; }
    uint32_t GetSize() const { return m_Size; }
    
    // Layout is crucial for VAO to know how to interpret this buffer
    const BufferLayout& GetLayout() const { return m_Layout; }
    void SetLayout(const BufferLayout& layout) { m_Layout = layout; }

private:
    uint32_t m_RendererID = 0;
    uint32_t m_Size = 0;
    void* m_MappedPointer = nullptr;
    BufferLayout m_Layout;
    bool m_IsStorage = false; // True if created via glBufferStorage (Immutable)
};