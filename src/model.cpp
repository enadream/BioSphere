#include "model.hpp"

ShaderTextureRange::ShaderTextureRange(const char * vert_sh_dir, const char * frag_sh_dir, uint32_t start, uint32_t size) : 
   m_Shader(vert_sh_dir, frag_sh_dir), TextureBegin(start), TextureAmount(size) {

}

Model::Model() : m_TotalIndicies(0){}

Model::Model(Model&& other) noexcept : m_Meshes(std::move(other.m_Meshes)), m_Texures(std::move(other.m_Texures)),
    m_Directory(std::move(other.m_Directory)), m_GammaCorrection(other.m_GammaCorrection), m_TextureMap(std::move(other.m_TextureMap)) {

}
void Model::Draw(){
    for (uint32_t i = 0; i < m_Meshes.size(); i++){
        m_Meshes[i].Draw();
    }
}
void Model::LoadAllMeshes(){
    for (uint32_t i = 0; i < m_Meshes.size(); i++){
        m_Meshes[i].LoadMesh();
    }
}
void Model::UnloadAllMeshes(){
    for (uint32_t i = 0; i < m_Meshes.size(); i++){
        m_Meshes[i].UnloadMesh();
    }
}

/// @brief this function loads the model from disk
/// @param path
void Model::LoadModel(const char *path){
    // read file via ASSIMP
    Assimp::Importer aiImporter;
    constexpr uint32_t flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace;
    const aiScene *scene = aiImporter.ReadFile(path, flags);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        printf("[ERROR]::ASSIMP:: %s\n", aiImporter.GetErrorString());
        return;
    }
    // retrieve the directory path of the filepath
    m_Directory = path;
    m_Directory.resize(m_Directory.find_last_of('/'));

    // process ASSIMP's root node recursively;
    processNode(scene->mRootNode, scene);
    printf("[INFO]: Total number of vertices in model %u\n", m_TotalIndicies);
}

// process a node in a recursive fashion
void Model::processNode(aiNode *ai_node, const aiScene *ai_scene){
    // process each mesh located aat the current node
    for (uint32_t i = 0; i < ai_node->mNumMeshes; i++){
        aiMesh* mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
        processMesh(mesh, ai_scene);
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (uint32_t i = 0; i < ai_node->mNumChildren; i++){
        processNode(ai_node->mChildren[i], ai_scene);
    }
}

void Model::processMesh(aiMesh *ai_mesh, const aiScene *ai_scene){
    Mesh tempMesh;

    tempMesh.m_Vertices.reserve(ai_mesh->mNumVertices);
    // walk through each of the mesh's vertices
    for (uint32_t i = 0; i < ai_mesh->mNumVertices; i++){
        // create new mesh
        tempMesh.m_Vertices.emplace_back();
        // position
        tempMesh.m_Vertices.back().Position.x = ai_mesh->mVertices[i].x;
        tempMesh.m_Vertices.back().Position.y = ai_mesh->mVertices[i].y;
        tempMesh.m_Vertices.back().Position.z = ai_mesh->mVertices[i].z;
        // normals
        if (ai_mesh->HasNormals()){
            tempMesh.m_Vertices.back().Normal.x = ai_mesh->mNormals[i].x;
            tempMesh.m_Vertices.back().Normal.y = ai_mesh->mNormals[i].y;
            tempMesh.m_Vertices.back().Normal.z = ai_mesh->mNormals[i].z;
        } else {
            tempMesh.m_Vertices.back().Normal = glm::vec3(0.0f);
        }
        // texture coordinates
        if (ai_mesh->mTextureCoords[0]){
            tempMesh.m_Vertices.back().TexCoords.x = ai_mesh->mTextureCoords[0][i].x;
            tempMesh.m_Vertices.back().TexCoords.y = ai_mesh->mTextureCoords[0][i].y;
            // TODO: Add tangent and bitangent
            // tangents
            // bitangent
        }
        else {
            tempMesh.m_Vertices.back().TexCoords = glm::vec2(0.0f);
        }
    }
    tempMesh.m_Indices.reserve(ai_mesh->mNumFaces*3);
    // walk through each of the mesh's faces
    for (uint32_t i = 0; i < ai_mesh->mNumFaces; i++){
        aiFace face = ai_mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++){
            tempMesh.m_Indices.emplace_back(face.mIndices[j]);
        }
    }
    m_TotalIndicies += tempMesh.m_Indices.size();
    // process materials
    aiMaterial *material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];

    // 1. diffuse maps
    processTexture(material, aiTextureType_DIFFUSE, TextureType::DIFFUSE);
    // 2. specular maps
    processTexture(material, aiTextureType_SPECULAR, TextureType::SPECULAR);
    // 3. normal maps
    processTexture(material, aiTextureType_HEIGHT, TextureType::NORMAL);
    // 4. height maps
    processTexture(material, aiTextureType_AMBIENT, TextureType::HEIGHT);

    // move temp data to the meshes
    m_Meshes.emplace_back(std::move(tempMesh));
}

void Model::processTexture(aiMaterial *ai_mat, aiTextureType ai_type, TextureType texture_type){
    for (uint32_t i = 0; i < ai_mat->GetTextureCount(ai_type); i++){
        aiString str;
        ai_mat->GetTexture(ai_type, i, &str);
        // check if file's name is not in the map or it's removed
        string textureName = str.C_Str();
        if (m_TextureMap.count(textureName) == 0 || m_TextureMap[textureName] == false){
            m_TextureMap[textureName] = true;
            // find full path
            textureName = m_Directory + "/" + textureName;
            m_Texures.emplace_back(textureName.c_str(), texture_type);
        }
    }
}

// You have to use shader before calling that function
void Model::LoadTextures(Shader &shader){
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
            uniformName += "DiffuseTexture";
            uniformName += numOfDiffuse++ == 0 ? "" : std::to_string(numOfDiffuse);
            break;
        case TextureType::SPECULAR:
            uniformName += "SpecularTexture";
            uniformName += numOfSpecular++ == 0 ? "" : std::to_string(numOfSpecular);
            break;
        case TextureType::NORMAL:
            uniformName += "NormalTexture";
            uniformName += numOfNormal++ == 0 ? "" : std::to_string(numOfNormal);
            break;
        case TextureType::HEIGHT:
            uniformName += "HeightTexture";
            uniformName += numOfHeight++ == 0 ? "" : std::to_string(numOfHeight);
            break;
        case TextureType::EMISSION:
            uniformName += "EmissionTexture";
            uniformName += numOfEmission++ == 0 ? "" : std::to_string(numOfEmission);
            break;
        default:
            printf("[ERROR]: Drawing failed. No type declared for the texture.\n");
            return;
        }
        
        // bind texture to the slot
        m_Texures[i].BindTo(i);
        shader.SetUniform1i(uniformName, i);
    }
}