#ifndef MESH_HPP
#define MESH_HPP

#include <glm/glm.hpp>
#include <vector>
using std::vector;

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};


class Mesh {
public: // functions
    Mesh();
    ~Mesh();

    // delete copy constructors
    Mesh(const Mesh &) = delete;
    Mesh& operator=(const Mesh &) = delete;
    // move constructor
    Mesh(Mesh && other) noexcept;

    void Draw();
    void LoadMesh();
    void UnloadMesh();

    void VertexToString();
    void PrintVertexBuffer();

public: // variables
    // mesh data
    vector<Vertex> m_Vertices;
    vector<uint32_t> m_Indices;

private: // variables
    uint32_t m_VAO, m_VBO, m_EBO;
    bool m_IsLoaded;
};


#endif