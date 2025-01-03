#include <string>
#include "mesh.hpp"

using std::string;

Mesh::Mesh() : m_VBO(0), m_VAO(0), m_EBO(0), m_IsLoaded(false){

}
Mesh::~Mesh(){
    // delete vertex buffer object
    glDeleteBuffers(1, VBO);
    // delete element buffer object
    glDeleteBuffers(1, EBO);
    // delete vertex array object
    glDeleteVertexArrays(1, VAO);
}

Mesh::LoadMesh(){
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
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        // loading index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size()*sizeof(uint32_t), &m_Indices[0], GL_STATIC_DRAW);;

        // unbind
        glBindVertexArray(0);
        m_IsLoaded = true;
    } else {
        printf("[WARNING]: Mesh has already loaded. Please unload it first and then load.");
    }
    
}
Mesh::UnloadMesh(){
    if (m_IsLoaded){
        // delete vertex buffer object
        glDeleteBuffers(1, VBO);
        // delete element buffer object
        glDeleteBuffers(1, EBO);
        // delete vertex array object
        glDeleteVertexArrays(1, VAO);
    }
    else {
        printf("[WARNING]: Mesh has already unloaded. There is nothing to unload.");
    }
}

// You have to use shader before calling that function
void Mesh::Draw(Shader &shader){
    // texture format = u_Material.DiffuseTexture + 0,1,2,...
    uint8_t numOfDiffuse = 0;
    uint8_t numOfSpecular = 0;
    uint8_t numOfNormal = 0;
    uint8_t numOfEmission = 0;
    uint8_t numOfHeight = 0;

    for (uint32_t i = 0; i < m_Texures.size(); i++){
        string uniformName = "u_Material." ; // DiffuseTexture" + std::to_string(i);
        
        switch (m_Texures[i].m_Type){
        case TextureType::DIFFUSE:
            uniformName += "DiffuseTexture" + std::to_string(numOfDiffuse++);
            break;
        case TextureType::SPECULAR:
            uniformName += "SpecularTexture" + std::to_string(numOfSpecular++);
            break;
        case TextureType::NORMAL:
            uniformName += "NormalTexture" + std::to_string(numOfNormal++);
            break;
        case TextureType::HEIGHT:
            uniformName += "HeightTexture" + std::to_string(numOfHeight++);
            break;
        case TextureType::EMISSION:
            uniformName += "EmissionTexture" + std::to_string(numOfEmission++);
            break;
        default:
            printf("[ERROR]: Drawing failed. No type declared for the texture.\n");
            return;
        }

        // bind texture to the slot
        m_Texures[i].BindTo(i);
        shader.SetUniform1i(uniformName.c_str(), i);
    }

    // draw mesh
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, 0);
}