#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <glad/glad.h>

#define MAX_TEXTURE_PATH_SIZE 127

enum TextureType : uint8_t {
    UNDEFINED,
    DIFFUSE,
    SPECULAR,
    NORMAL,
    HEIGHT,
    EMISSION,
};


class Texture {
public: // methods
    Texture(const char* path, TextureType type = DIFFUSE);
    ~Texture();


    inline void BindTo(uint8_t slot_id){
        glActiveTexture(GL_TEXTURE0 + slot_id);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
    }
    inline uint32_t GetID() {
        return m_TextureID;
    }

public: // variables
    int32_t m_Width, m_Height, m_NumOfChannels;
    char m_Path[MAX_TEXTURE_PATH_SIZE];
    TextureType m_Type;

private: // variables
    uint32_t m_TextureID;
};



#endif