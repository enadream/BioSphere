#ifndef MODEL_HPP
#define MODEL_HPP

#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <unordered_map>
#include <vector>
#include <string>
using std::vector, std::string, std::unordered_map;

#include "mesh.hpp"
#include "texture.hpp"
#include "shader.hpp"

struct ShaderTextureRange {
    Shader m_Shader;
    uint32_t TextureBegin;
    uint32_t TextureAmount;

    ShaderTextureRange(const char * vert_sh_dir, const char * frag_sh_dir, uint32_t start, uint32_t size);
};

class Model {
public: // functions
    Model();

    // delete copy constructors
    Model(const Model &) = delete;
    Model& operator=(const Model &) = delete;
    // move constructor
    Model(Model&& other) noexcept;

    void Draw();
    void LoadModel(const char *path);
    void LoadTextures(Shader &shader);
    void LoadAllMeshes();
    void UnloadAllMeshes();
    

public: // variables
    vector<Mesh> m_Meshes;
    vector<Texture> m_Texures;
    string m_Directory;
    bool m_GammaCorrection;

private: // variables
    unordered_map<string, bool> m_TextureMap;
    uint32_t m_TotalIndicies;

private: // functions
    void processNode(aiNode *ai_node, const aiScene *ai_scene);
    void processMesh(aiMesh *ai_mesh, const aiScene *ai_scene);
    void processTexture(aiMaterial *ai_mat, aiTextureType ai_type, TextureType texture_type);
};


#endif
