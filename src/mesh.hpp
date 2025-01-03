#ifndef MESH_HPP
#define MESH_HPP

#include <vector>
#include "texture.hpp"
#include "shader.hpp"

using std::vector;

struct Vertex
{
    float Position[3];
    float Normal[3];
    float TexCoords[2];
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    void Draw(Shader &shader);
    void LoadMesh();
    void UnloadMesh();
public:
    // mesh data
    vector<Vertex> m_Vertices;
    vector<uint32_t> m_Indices;
    vector<Texture> m_Texures;

private:
    uint32_t m_VAO, m_VBO, m_EBO;
    bool m_IsLoaded;
};


#endif