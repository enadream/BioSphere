#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <glad/glad.h>
#include <string>
#include <vector>

using std::string, std::vector;

#define MAX_TEXTURE_PATH_SIZE 127

enum class TextureType : uint8_t {
    UNDEFINED,
    DIFFUSE,
    SPECULAR,
    NORMAL,
    HEIGHT,
    EMISSION,
};

// base class
class BaseTexture {
public: // methods
    BaseTexture();
    ~BaseTexture();
    
    BaseTexture(const BaseTexture&) = delete;
    BaseTexture& operator=(const BaseTexture&) = delete;
    // Move constructor
    BaseTexture(BaseTexture&& other) noexcept;

    void UnloadTexture();

    inline uint32_t GetID() const {
        return m_TextureID;
    }

public: // variables
    string m_Path;
    int32_t m_Width, m_Height, m_NumOfChannels;
    TextureType m_Type;

protected: // variables
    uint32_t m_TextureID;
    bool m_IsLoaded;
};

class Texture : public BaseTexture {
public: // functions
    Texture(const char* path, TextureType type = TextureType::UNDEFINED);
    Texture();

    // Move constructor
    Texture(Texture&& other) noexcept;

    void LoadTexture(const char* path, TextureType type = TextureType::UNDEFINED);

    inline void BindTo(uint8_t slot_id) const {
        glActiveTexture(GL_TEXTURE0 + slot_id);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
    }
};

class CubeTexture : public BaseTexture {
public: // functions
    CubeTexture(const vector<string> &faces, TextureType type = TextureType::UNDEFINED);
    CubeTexture();

    // Move constructor
    CubeTexture(CubeTexture&& other) noexcept;

    void LoadCubeMap(const vector<string> &faces, TextureType type = TextureType::UNDEFINED);

    inline void BindTo(uint8_t slot_id) const {
        glActiveTexture(GL_TEXTURE0 + slot_id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);
    }
};


#endif