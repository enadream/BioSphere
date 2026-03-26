#include "renderer/shader.hpp"
#include "core/log.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <vector>

// --- Constructors ---

Shader::Shader(const std::string& computePath) 
    : m_Name(computePath)
{
    std::string source = ReadFile(computePath);
    uint32_t computeShader = CompileShader(GL_COMPUTE_SHADER, source);
    LinkCompute(computeShader);
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
    : m_Name(vertexPath)
{
    std::string vertexSource = ReadFile(vertexPath);
    std::string fragmentSource = ReadFile(fragmentPath);

    uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
    uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    LinkProgram(vs, fs);
}

Shader::Shader(const std::string& vertexPath, const std::string& geometryPath, const std::string& fragmentPath)
    : m_Name(vertexPath)
{
    std::string vertexSource = ReadFile(vertexPath);
    std::string geometrySource = ReadFile(geometryPath);
    std::string fragmentSource = ReadFile(fragmentPath);

    uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
    uint32_t gs = CompileShader(GL_GEOMETRY_SHADER, geometrySource);
    uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    LinkProgram(vs, fs, gs);
}

Shader::~Shader() {
    if (m_RendererID) glDeleteProgram(m_RendererID);
}

// Move Semantics
Shader::Shader(Shader&& other) noexcept 
    : m_RendererID(other.m_RendererID), 
      m_Name(std::move(other.m_Name)), 
      m_UniformLocationCache(std::move(other.m_UniformLocationCache))
{
    other.m_RendererID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_RendererID) glDeleteProgram(m_RendererID);
        m_RendererID = other.m_RendererID;
        m_Name = std::move(other.m_Name);
        m_UniformLocationCache = std::move(other.m_UniformLocationCache);
        other.m_RendererID = 0;
    }
    return *this;
}

void Shader::Bind() const {
    glUseProgram(m_RendererID);
}

void Shader::Unbind() const {
    glUseProgram(0);
}

// --- Uniforms ---

void Shader::SetInt(const std::string& name, int value) {
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetIntArray(const std::string& name, int* values, uint32_t count) {
    glUniform1iv(GetUniformLocation(name), count, values);
}

void Shader::SetFloat(const std::string& name, float value) {
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetFloat2(const std::string& name, const glm::vec2& value) {
    glUniform2f(GetUniformLocation(name), value.x, value.y);
}

void Shader::SetFloat3(const std::string& name, const glm::vec3& value) {
    glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
}

void Shader::SetFloat4(const std::string& name, const glm::vec4& value) {
    glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::SetMat3(const std::string& name, const glm::mat3& matrix) {
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::SetMat4(const std::string& name, const glm::mat4& matrix) {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::SetUniform1i(int location, int value) {
    glUniform1i(location, value);
}

void Shader::SetUniform1f(int location, float value) {
    glUniform1f(location, value);
}

int Shader::GetUniformLocation(const std::string& name) {
    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
        return m_UniformLocationCache[name];

    int location = glGetUniformLocation(m_RendererID, name.c_str());
    if (location == -1) {
        // LOG_WARN("Shader Warning: Uniform '%s' not found or compiled out.", name.c_str());
    }
    
    m_UniformLocationCache[name] = location;
    return location;
}

// --- Internal Utilities ---

std::string Shader::ReadFile(const std::string& filepath) {
    std::string result;
    std::ifstream file(filepath, std::ios::in | std::ios::binary); 
    if (file) {
        file.seekg(0, std::ios::end);
        std::streampos size = file.tellg();
        if (size != -1) {
            result.resize(size);
            file.seekg(0, std::ios::beg);
            file.read(&result[0], size);
        } else {
            LOG_ERROR("Could not read from file '%s'", filepath.c_str());
        }
    } else {
        LOG_ERROR("Could not open file '%s'", filepath.c_str());
    }
    return result;
}

uint32_t Shader::CompileShader(uint32_t type, const std::string& source) {
    uint32_t id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        
        const char* typeName = (type == GL_VERTEX_SHADER ? "Vertex" : 
                               (type == GL_FRAGMENT_SHADER ? "Fragment" : 
                               (type == GL_GEOMETRY_SHADER ? "Geometry" : "Compute")));
        
        LOG_ERROR("Failed to compile %s Shader in '%s'", typeName, m_Name.c_str());
        LOG_ERROR("%s", message);
        
        glDeleteShader(id);
        return 0;
    }
    return id;
}

void Shader::LinkProgram(uint32_t vertexShader, uint32_t fragmentShader, uint32_t geometryShader) {
    m_RendererID = glCreateProgram();
    glAttachShader(m_RendererID, vertexShader);
    if (geometryShader) glAttachShader(m_RendererID, geometryShader);
    glAttachShader(m_RendererID, fragmentShader);
    
    glLinkProgram(m_RendererID);

    // Validation
    int isLinked = 0;
    glGetProgramiv(m_RendererID, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        int maxLength = 0;
        glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &maxLength);
        std::vector<char> infoLog(maxLength);
        glGetProgramInfoLog(m_RendererID, maxLength, &maxLength, &infoLog[0]);
        
        LOG_ERROR("Shader Link Failure (%s):", m_Name.c_str());
        LOG_ERROR("%s", infoLog.data());
        
        glDeleteProgram(m_RendererID);
        m_RendererID = 0;
    }

    // Always detach and delete shaders after linking; they are stored in the program now.
    glDetachShader(m_RendererID, vertexShader);
    glDeleteShader(vertexShader);
    
    if (geometryShader) {
        glDetachShader(m_RendererID, geometryShader);
        glDeleteShader(geometryShader);
    }

    glDetachShader(m_RendererID, fragmentShader);
    glDeleteShader(fragmentShader);
}

void Shader::LinkCompute(uint32_t computeShader) {
    m_RendererID = glCreateProgram();
    glAttachShader(m_RendererID, computeShader);
    glLinkProgram(m_RendererID);

    int isLinked = 0;
    glGetProgramiv(m_RendererID, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        int maxLength = 0;
        glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &maxLength);
        std::vector<char> infoLog(maxLength);
        glGetProgramInfoLog(m_RendererID, maxLength, &maxLength, &infoLog[0]);
        
        LOG_ERROR("Compute Shader Link Failure (%s):", m_Name.c_str());
        LOG_ERROR("%s", infoLog.data());
        
        glDeleteProgram(m_RendererID);
        m_RendererID = 0;
    }

    glDetachShader(m_RendererID, computeShader);
    glDeleteShader(computeShader);
}