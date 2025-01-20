#include <glad/glad.h> 

#include "mesh.hpp"
#include "debug_func.hpp"

Mesh::Mesh() :  m_VAO(0), m_VBO(0), m_EBO(0), m_IsLoaded(false) {}

Mesh::Mesh(Mesh&& other) noexcept : m_Vertices(std::move(other.m_Vertices)), m_Indices(std::move(other.m_Indices)),
    m_VAO(other.m_VAO), m_VBO(other.m_VBO), m_EBO(other.m_EBO), m_IsLoaded(other.m_IsLoaded) {
    other.m_VAO = 0; // invalidate other mesh
    other.m_VBO = 0; // invalidate other mesh
    other.m_EBO = 0; // invalidate other mesh
}

Mesh::~Mesh(){
    // delete vertex buffer object
    glDeleteBuffers(1, &m_VBO);
    // delete element buffer object
    glDeleteBuffers(1, &m_EBO);
    // delete vertex array object
    glDeleteVertexArrays(1, &m_VAO);
}

void Mesh::LoadMesh(){
    if (!m_IsLoaded){
        // generate array object and buffers
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        // bindings
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, m_Vertices.size()*sizeof(Vertex), &m_Vertices[0], GL_STATIC_DRAW);
        // vertex position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        // loading index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size()*sizeof(uint32_t), &m_Indices[0], GL_STATIC_DRAW);

        // unbind
        glBindVertexArray(0);
        m_IsLoaded = true;
        
    } else {
        printf("[ERROR]: Mesh has already loaded. Please unload it first and then load.");
    }
}
void Mesh::UnloadMesh(){
    if (m_IsLoaded){
        // delete vertex buffer object
        glDeleteBuffers(1, &m_VBO);
        // delete element buffer object
        glDeleteBuffers(1, &m_EBO);
        // delete vertex array object
        glDeleteVertexArrays(1, &m_VAO);
    }
    else {
        printf("[ERROR]: Mesh has already unloaded. There is nothing to unload.");
    }
}

void Mesh::Draw(){
    // draw mesh
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, 0);
    // glDrawArrays(GL_TRIANGLES, 0, m_Vertices.size());
}


void Mesh::VertexToString(){
    for (int i = 0; i < m_Vertices.size(); i++){
            printf("VERTEX %i : %f, %f, %f, %f, %f, %f, %f, %f\n", i,
                m_Vertices[i].Position[0], m_Vertices[i].Position[1], m_Vertices[i].Position[2],
                m_Vertices[i].Normal[0], m_Vertices[i].Normal[1], m_Vertices[i].Normal[2], 
                m_Vertices[i].TexCoords[0], m_Vertices[i].TexCoords[1]);
    }
}

#define VERTEX_FLOAT_AMOUNT (sizeof(Vertex)/sizeof(float))

void Mesh::PrintVertexBuffer(){
    // bind first !!!
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    std::vector<float> bufferData(m_Vertices.size() * VERTEX_FLOAT_AMOUNT); // Adjust size as needed
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferData.size() * sizeof(float), bufferData.data());

    for (int i = 0; i < bufferData.size(); i++) {
        printf("%f, ", bufferData[i]);
        if ((i+1) % VERTEX_FLOAT_AMOUNT == 0)
            printf("\n");
    }
}