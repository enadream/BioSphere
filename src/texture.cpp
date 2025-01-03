#include "texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(const char* path, TextureType type) : m_Type(type) {
    // generating texture
    glGenTextures(1, &m_TextureID);
    glBindTexture(GL_TEXTURE_2D, m_TextureID);

    // set the texture wrapping/filtering options (on the currently bounded texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load and generate the texture
    stbi_set_flip_vertically_on_load(true);
    char * buffer = stbi_load(path, &m_Width, &m_Height, &m_NumOfChannels, 0);

    if (buffer){
        int32_t type;
        switch (m_NumOfChannels)
        {
        case 4:
            type = GL_RGBA;
            break;
        case 3:
            type = GL_RGB;
            break;
        case 2:
            type = GL_RG;
            break;
        case 1:
            type = GL_RED;
            break;
        default:
            printf("[ERROR]: Texture data dimensions are wrong!\n");
            return;
        }
        // load data from buffer
        glTexImage2D(GL_TEXTURE_2D, 0, type, m_Width, m_Height, 0, type, GL_UNSIGNED_BYTE, buffer);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(buffer);
    } else {
        printf("[ERROR]: Texture data couldn't loaded. Check the path!\n");
    }
}

Texture::~Texture(){
    glDeleteTextures(1, &m_TextureID);
}