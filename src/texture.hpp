#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

class Texture {
public:
    Texture(const char* path);
    ~Texture();

    void BindTo(uint8_t slot_id);
private:
    uint32_t m_TextureID;
    int32_t m_Width, m_Height, m_NumOfChannels;
    uint8_t* m_Buffer;
};



#endif