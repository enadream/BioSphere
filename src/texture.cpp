#include "texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

//////////// Texture ////////////
Texture::Texture(GLenum gl_type, TextureType tex_type) : 
    m_Width(0), m_Height(0), m_Format(0), m_GLType(gl_type), m_Type(tex_type), m_Allocated(false) {
    glGenTextures(1, &m_TextureID);
}
Texture::~Texture() {
    glDeleteTextures(1, &m_TextureID);
}
Texture::Texture(Texture &&other) noexcept : 
    m_Width(other.m_Width), m_Height(other.m_Height), m_Format(other.m_Format), m_TextureID(other.m_TextureID),
    m_GLType(other.m_GLType), m_Type(other.m_Type) {
    // falsify texture ID
    other.m_TextureID = 0;
}
void Texture::Generate(GLenum gl_type, TextureType tex_type){
    glGenTextures(1, &m_TextureID);
    m_GLType = gl_type;
    m_Type = tex_type;
}
void Texture::Free() {
    glDeleteTextures(1, &m_TextureID);
    m_TextureID = 0;
    m_Allocated = false;
    m_Width = 0;
    m_Height = 0;
}
void Texture::SetTexParametrI(GLenum pname, GLint param) {
    glTexParameteri(m_GLType, pname, param);
}
void Texture::GLTexImage2D(GLint level, GLint internalFormat, int32_t width, int32_t height, GLenum format, GLenum type, const void * data){
    m_Width = width;
    m_Height = height;
    m_Format = internalFormat;
    glBindTexture(m_GLType, m_TextureID);
    glTexImage2D(m_GLType, level, internalFormat, width, height, 0, format, type, data);
}
void Texture::GLTexStorage2D(GLint level, GLenum internalFormat, int32_t width, int32_t height){
    m_Width = width;
    m_Height = height;
    m_Format = internalFormat;
    glBindTexture(m_GLType, m_TextureID);
    glTexStorage2D(m_GLType, level, internalFormat, width, height);
}
void Texture::LoadFromFile(const char* path){
    if (!m_Allocated){
        // load and generate the texture
        stbi_set_flip_vertically_on_load(true);
        int32_t numOfChannel;
        uint8_t * buffer = stbi_load(path, &m_Width, &m_Height, &numOfChannel, 0);

        if (buffer){
            // generating texture
            glBindTexture(m_GLType, m_TextureID);
            int32_t channelType;
            switch (numOfChannel)
            {
            case 4:
                channelType = GL_RGBA;
                break;
            case 3:
                channelType = GL_RGB;
                break;
            case 2:
                channelType = GL_RG;
                break;
            case 1:
                channelType = GL_RED;
                break;
            default:
                printf("[ERROR]: Texture data dimensions are wrong!\n");
                return;
            }
            // load data from buffer
            glTexImage2D(m_GLType, 0, channelType, m_Width, m_Height, 0, channelType, GL_UNSIGNED_BYTE, buffer);
            glGenerateMipmap(m_GLType);
            stbi_image_free(buffer);
            m_Allocated = true;
            // unbind
            glBindTexture(m_GLType, 0);
        } else {
            printf("[ERROR]: Texture data couldn't loaded. Check the path=\"%s\" \n", path);
            m_TextureID = 0;
            m_Allocated = false;
        }
    } else {
        printf("[ERROR]: The texture already allocated. New allocation is not permitted! path=\"%s\"\n", path);
    }
}

//////////// BaseTexture ////////////

BaseTexture::BaseTexture() : m_TextureID(0), m_IsLoaded(false) {}

BaseTexture::~BaseTexture(){
    glDeleteTextures(1, &m_TextureID);
}
BaseTexture::BaseTexture(BaseTexture&& other) noexcept : m_Path(std::move(other.m_Path)), m_Width(other.m_Width), m_Height(other.m_Height),
    m_NumOfChannels(other.m_NumOfChannels), m_Type(other.m_Type), m_TextureID(other.m_TextureID), m_IsLoaded(other.m_IsLoaded){
    other.m_TextureID = 0; // Invalidate the moved-from object's OpenGL texture
}
void BaseTexture::UnloadTexture(){
    if(m_IsLoaded){
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
        m_IsLoaded = false;
    } else {
        printf("[ERROR]: Texture already unloaded. There is nothing to unload!\n");
    }
}

//////////// SimpleTexture ////////////
SimpleTexture::SimpleTexture(const char* path, TextureType type){
    LoadTexture(path, type);
}
SimpleTexture::SimpleTexture() : BaseTexture() {}
SimpleTexture::SimpleTexture(SimpleTexture&& other) noexcept : BaseTexture(std::move(other)) {}

void SimpleTexture::LoadTexture(const char* path, TextureType type){
    if (!m_IsLoaded){
        // update data
        m_Path = path;
        m_Type = type;

        // load and generate the texture
        stbi_set_flip_vertically_on_load(true);
        uint8_t * buffer = stbi_load(path, &m_Width, &m_Height, &m_NumOfChannels, 0);

        if (buffer){
            // generating texture
            glGenTextures(1, &m_TextureID);
            glBindTexture(GL_TEXTURE_2D, m_TextureID);

            // set the texture wrapping/filtering options (on the currently bounded texture object)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            int32_t channelType;
            switch (m_NumOfChannels)
            {
            case 4:
                channelType = GL_RGBA;
                break;
            case 3:
                channelType = GL_RGB;
                break;
            case 2:
                channelType = GL_RG;
                break;
            case 1:
                channelType = GL_RED;
                break;
            default:
                printf("[ERROR]: Texture data dimensions are wrong!\n");
                return;
            }
            // load data from buffer
            glTexImage2D(GL_TEXTURE_2D, 0, channelType, m_Width, m_Height, 0, channelType, GL_UNSIGNED_BYTE, buffer);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(buffer);
            m_IsLoaded = true;
            // unbind
            glBindTexture(GL_TEXTURE_2D, 0);
        } else {
            printf("[ERROR]: Texture data couldn't loaded. Check the path=\"%s\" \n", path);
            m_TextureID = 0;
            m_IsLoaded = false;
        }
    } else {
        printf("[ERROR]: Texture couldn't loaded. You should unload it first! path=\"%s\"\n", path);
    }
}

//////////// Cube Texture ////////////
CubeTexture::CubeTexture(const vector<string> &faces, TextureType type){
    LoadCubeMap(faces, type);
}
CubeTexture::CubeTexture() : BaseTexture() {}
CubeTexture::CubeTexture(CubeTexture&& other) noexcept : BaseTexture(std::move(other)){}

void CubeTexture::LoadCubeMap(const vector<string> &faces, TextureType type){
    if (!m_IsLoaded){
        // update data
        m_Type = type;

        glGenTextures(1, &m_TextureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);

        // setting parameters
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        for (uint32_t i = 0; i < faces.size(); i++){
            uint8_t * data = stbi_load(faces[i].c_str(), &m_Width, &m_Height, &m_NumOfChannels, 0);

            if (data){
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                    0, GL_RGB, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            }
            else {
                printf("[ERROR]: CubeMap texture couldn't loaded. Check the path=\"%s\" \n", faces[i].c_str());
                break;
            }
            stbi_image_free(data);
        }

        m_IsLoaded = true; // Mark as loaded
    } else {
        printf("[ERROR]: CubeMap texture couldn't loaded. You should unload it first! path=\"%s\"\n", m_Path.c_str());
    }
}

