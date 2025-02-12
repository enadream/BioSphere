#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <glad/glad.h>
#include <string>
#include <vector>

using std::string, std::vector;

enum class TextureType : uint8_t {
    UNDEFINED,
    DIFFUSE,
    SPECULAR,
    NORMAL,
    HEIGHT,
    EMISSION,
    IMAGE,
    DEPTH
};

class Texture {
public: // functions
    Texture(GLenum gl_type, TextureType tex_type);
    ~Texture();

    // copy constructors
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    // move constructor
    Texture(Texture &&other) noexcept;

    // you should set tex parameters first before calling this function!
    void LoadFromFile(const char* path);
    // always bind before using this function
    void SetTexParametrI(GLenum pname, GLint param);
    // automatically binds, reallocation allowed
    void GLTexImage2D(GLint level, GLint internalFormat, int32_t width, int32_t height, GLenum format, GLenum type, const void * data);
    void GLTexStorage2D(GLint level, GLint internalFormat, int32_t width, int32_t height);

    void Free();
    void Generate(GLenum gl_type, TextureType tex_type);
    inline void Bind() const {
        glBindTexture(m_GLType, m_TextureID);
    }
    inline void BindTo(uint8_t slot_id) const {
        glActiveTexture(GL_TEXTURE0 + slot_id);
        glBindTexture(m_GLType, m_TextureID);
    }
    inline void BindImageTexture(GLuint unit, GLint level, GLboolean layered, GLint layer, GLenum access) const {
        glBindImageTexture(unit, m_TextureID, level, layered, layer, access, m_Format);
    }
    inline void Unbind() const {
        glBindTexture(m_GLType, 0);
    }
    inline uint32_t GetID() const {
        return m_TextureID;
    }
    inline uint32_t GetWidth(){
        return m_Width;
    }
    inline uint32_t GetHeight(){
        return m_Height;
    }
    inline GLenum GetType(){
        return m_GLType;
    }
public: // variables
private: // variables
    int32_t m_Width, m_Height, m_Format;
    uint32_t m_TextureID;
    GLenum m_GLType;
    TextureType m_Type;
    bool m_Allocated;
private: // functions

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

class SimpleTexture : public BaseTexture {
public: // functions
    SimpleTexture(const char* path, TextureType type = TextureType::UNDEFINED);
    SimpleTexture();

    // Move constructor
    SimpleTexture(SimpleTexture&& other) noexcept;

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