#include <stdio.h>
#include "index_buffer.hpp"

IndexBuffer::IndexBuffer(uint32_t size, const void*data, IndexType data_type, GLenum usage) : m_DataType(data_type){
    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);

    switch (data_type)
    {
    case IndexType::UINT8:
        m_Count = size / sizeof(uint8_t);
        break;
    case IndexType::UINT16:
        m_Count = size / sizeof(uint16_t);
        break;
    case IndexType::UINT32:
        m_Count = size / sizeof(uint32_t);
        break;
    default:
        printf("[ERROR]: The index buffer's type is undefined!\n");
        break;
    }
}

IndexBuffer::IndexBuffer() : m_EBO(0), m_DataType(IndexType::UNDEFINED), m_Count(0) {}

IndexBuffer::~IndexBuffer() {
    glDeleteBuffers(1, &m_EBO);
}

IndexBuffer::IndexBuffer(IndexBuffer && other) noexcept : 
    m_EBO(other.m_EBO), m_DataType(other.m_DataType), m_Count(other.m_Count) {
    other.m_EBO = 0;
}

void IndexBuffer::GenIndexBuffer(uint32_t size, const void*data, IndexType data_type, GLenum usage){
    if (m_EBO == 0){
        glGenBuffers(1, &m_EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);

        m_DataType = data_type;

        switch (data_type)
        {
        case IndexType::UINT8:
            m_Count = size / sizeof(uint8_t);
            break;
        case IndexType::UINT16:
            m_Count = size / sizeof(uint16_t);
            break;
        case IndexType::UINT32:
            m_Count = size / sizeof(uint32_t);
            break;
        default:
            printf("[ERROR]: The index buffer's type is undefined!\n");
            break;
        }
    }
    else {
        printf("[ERROR]: Index buffer couldn't generated because Index buffer already generated!.\n");
    }
}