#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <glad/glad.h>

// Defines the usage of the texture in the shader (Material system)
enum class TextureType : uint8_t {
    None = 0, Diffuse, Specular, Normal, Height, Emission, Image, Depth, AmbientOcclusion, Light
};

// Configuration for creating a texture manually (e.g., for FBO attachments)
struct TextureSpecification {
    uint32_t Width = 1;
    uint32_t Height = 1;
    GLenum InternalFormat = GL_RGBA8; // How GPU stores it
    GLenum DataFormat = GL_RGBA;      // How we provide it
    GLenum WrapS = GL_REPEAT;
    GLenum WrapT = GL_REPEAT;
    GLenum MinFilter = GL_LINEAR_MIPMAP_LINEAR;
    GLenum MagFilter = GL_LINEAR;
    bool GenerateMips = true;
};

// Abstract Base Class
class Texture {
public:
    virtual ~Texture() = default;
    
    // Getters are now non-virtual since data is stored in base
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    uint32_t GetRendererID() const { return m_RendererID; }
    
    virtual void Bind(uint32_t slot = 0) const = 0;
    virtual void Unbind() const = 0;

    virtual bool operator==(const Texture& other) const {
        return m_RendererID == other.m_RendererID;
    }

protected:
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_RendererID = 0;
};

// 2D Texture (Standard Images)
class Texture2D : public Texture {
public:
    // Case 1: Load from Disk (Asset)
    Texture2D(const std::string& path, TextureType type = TextureType::None, 
        const TextureSpecification& spec = TextureSpecification());

    // Case 2: Create from Specification (Framebuffer / Procedural)
    Texture2D(const TextureSpecification& spec);

    virtual ~Texture2D();

    // RAII: Delete copy, allow move
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    // Upload raw data to the texture (useful for procedural generation)
    void SetData(void* data, uint32_t size);
    virtual void Bind(uint32_t slot = 0) const override { glBindTextureUnit(slot, m_RendererID); };
    virtual void Unbind() const override { glBindTexture(GL_TEXTURE_2D, 0); }

    TextureType GetType() const { return m_Type; }
    const std::string& GetPath() const { return m_Path; }

private:
    std::string m_Path;
    GLenum m_InternalFormat = 0, m_DataFormat = 0;
    TextureType m_Type = TextureType::None;
};

// Cube Map Texture (Skyboxes)
class TextureCube : public Texture {
public:
    // Order: Right, Left, Top, Bottom, Back, Front
    TextureCube(const std::vector<std::string>& faces);
    virtual ~TextureCube();

    // RAII: Delete copy, allow move
    TextureCube(const TextureCube&) = delete;
    TextureCube& operator=(const TextureCube&) = delete;
    TextureCube(TextureCube&& other) noexcept;
    TextureCube& operator=(TextureCube&& other) noexcept;

    virtual void Bind(uint32_t slot = 0) const override { glBindTextureUnit(slot, m_RendererID); }
    virtual void Unbind() const override { glBindTexture(GL_TEXTURE_CUBE_MAP, 0); }
};