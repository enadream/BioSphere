#include "renderer/texture.hpp"
#include "core/log.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cmath>     // For log2, floor
#include <algorithm> // For max

// --- Texture2D Implementation ---

// Case 1: Load from File
Texture2D::Texture2D(const std::string& path, TextureType type, const TextureSpecification& spec) 
    : m_Path(path), m_Type(type)
{
    int width, height, numOfChannel;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* data = stbi_load(path.c_str(), &width, &height, &numOfChannel, 0);

    if (data){
        m_Width = width;
        m_Height = height;

        GLenum internalFormat = 0, dataFormat = 0;
        switch (numOfChannel)
            {
            case 4:
                internalFormat = GL_RGBA8;
                dataFormat = GL_RGBA;
                break;
            case 3:
                internalFormat = GL_RGB8;
                dataFormat = GL_RGB;
                break;
            case 2:
                internalFormat = GL_RG8;
                dataFormat = GL_RG;
                break;
            case 1:
                internalFormat = GL_R8;
                dataFormat = GL_RED;
                break;
            default:
                LOG_ERROR("[ERROR]: Texture data dimensions are wrong!");
                return;
        }

        m_InternalFormat = internalFormat;
        m_DataFormat = dataFormat;

        // Calculate Mip Levels based on spec
        GLsizei levels = 1;
        if (spec.GenerateMips) {
            levels = (GLsizei)std::floor(std::log2(std::max(m_Width, m_Height))) + 1;
        }

        // Direct State Access: Create and setup without binding
        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);

        // Allocate storage (Immutable is faster and safer)
        // Calculate MIP levels
        glTextureStorage2D(m_RendererID, levels, internalFormat, m_Width, m_Height);

        // Upload Data
        glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

        // Setup Parameters from Specification
        glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, spec.MinFilter);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, spec.MagFilter);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, spec.WrapS);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, spec.WrapT);

        // Generate the mips on the GPU if requested
        if (spec.GenerateMips) {
            glGenerateTextureMipmap(m_RendererID);
        }

        stbi_image_free(data);
        LOG_INFO("Loaded Texture: %s (%dx%d)", path.c_str(), m_Width, m_Height);
    } else {
        LOG_ERROR("Failed to load texture: %s", path.c_str());
    }
}

// Case 2: Create from Specification (e.g., FBO)
Texture2D::Texture2D(const TextureSpecification& spec)
    : m_InternalFormat(spec.InternalFormat), m_DataFormat(spec.DataFormat)
{
    // Base class members must be assigned in body
    m_Width = spec.Width;
    m_Height = spec.Height;
    glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
    
    // Calculate Mip Levels based on specification
    GLsizei levels = 1;
    if (spec.GenerateMips) {
        levels = (GLsizei)std::floor(std::log2(std::max(m_Width, m_Height))) + 1;
    }

    // Allocate memory  Note: glTextureStorage2D is immutable storage.
    glTextureStorage2D(m_RendererID, levels, m_InternalFormat, m_Width, m_Height);

    // Setup parameters
    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, spec.MinFilter);
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, spec.MagFilter);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, spec.WrapS);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, spec.WrapT);
}

Texture2D::~Texture2D() {
    if (m_RendererID) glDeleteTextures(1, &m_RendererID);
}

// Move Constructor
Texture2D::Texture2D(Texture2D&& other) noexcept 
    : m_Path(std::move(other.m_Path)), 
      m_InternalFormat(other.m_InternalFormat), m_DataFormat(other.m_DataFormat),
      m_Type(other.m_Type)
{
    // Assign base members
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_RendererID = other.m_RendererID;

    other.m_RendererID = 0; // Prevent double deletion
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        if (m_RendererID) glDeleteTextures(1, &m_RendererID);
        
        m_Path = std::move(other.m_Path);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_RendererID = other.m_RendererID;
        m_InternalFormat = other.m_InternalFormat;
        m_DataFormat = other.m_DataFormat;
        m_Type = other.m_Type;

        other.m_RendererID = 0;
    }
    return *this;
}

void Texture2D::SetData(void* data, uint32_t size) {
    // Calculate expected bytes per pixel
    uint32_t bpp = 4; // Default GL_RGBA
    switch (m_DataFormat) {
        case GL_RGB:
            bpp = 3;
            break;
        case GL_RG:
            bpp = 2;
            break;
        case GL_RED:
            bpp = 1;
            break;
        default:
            break;
    }

    uint32_t expectedSize = m_Width * m_Height * bpp;

    // Safety Check: Prevent buffer overflow or mismatch
    if (size != expectedSize) {
        LOG_ERROR("Texture2D::SetData Failed: Size mismatch! Expected %d bytes, got %d bytes.", expectedSize, size);
        return;
    }

    // Note: This only uploads to the base level (Mip 0).
    // If Mips are enabled, you likely want to call glGenerateTextureMipmap after this.
    glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
}


// --- TextureCube Implementation ---
TextureCube::TextureCube(const std::vector<std::string>& faces) {
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_RendererID);
    
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false); // Cubemaps don't usually need flipping

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            m_Width = width;
            m_Height = height;
            // Upload to specific face
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            LOG_ERROR("Cubemap texture failed to load: %s", faces[i].c_str());
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

TextureCube::~TextureCube() {
    glDeleteTextures(1, &m_RendererID);
}

TextureCube::TextureCube(TextureCube&& other) noexcept {
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_RendererID = other.m_RendererID;
    other.m_RendererID = 0;
}

TextureCube& TextureCube::operator=(TextureCube&& other) noexcept {
    if (this != &other) {
        if (m_RendererID) glDeleteTextures(1, &m_RendererID);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_RendererID = other.m_RendererID;
        other.m_RendererID = 0;
    }
    return *this;
}