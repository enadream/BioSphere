#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

// Abstracted Shader class handling compilation, linking, and uniforms.
class Shader {
public:
    // Case 1: Compute Shader
    Shader(const std::string& computePath);
    
    // Case 2: Vertex + Fragment Shader
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    
    // Case 3: Vert + Geo + Frag
    Shader(const std::string& vertexPath, const std::string& geometryPath, const std::string& fragmentPath);

    ~Shader();

    // RAII
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void Bind() const;
    void Unbind() const;

    uint32_t GetRendererID() const { return m_RendererID; }

    // --- Uniforms ---
    void SetInt(const std::string& name, int value);
    void SetIntArray(const std::string& name, int* values, uint32_t count);
    void SetFloat(const std::string& name, float value);
    void SetFloat2(const std::string& name, const glm::vec2& value);
    void SetFloat3(const std::string& name, const glm::vec3& value);
    void SetFloat4(const std::string& name, const glm::vec4& value);
    void SetMat3(const std::string& name, const glm::mat3& matrix);
    void SetMat4(const std::string& name, const glm::mat4& matrix);
    
    // Low-level direct access (if you cached the location yourself)
    void SetUniform1i(int location, int value);
    void SetUniform1f(int location, float value);

private:
    // Returns location or -1 if not found
    int GetUniformLocation(const std::string& name);
    
    // Helper to compile individual stages
    uint32_t CompileShader(uint32_t type, const std::string& source);
    // Helper to read file to string
    std::string ReadFile(const std::string& filepath);
    // Links program and validates
    void LinkProgram(uint32_t vertexShader, uint32_t fragmentShader, uint32_t geometryShader = 0);
    void LinkCompute(uint32_t computeShader);

private:
    uint32_t m_RendererID = 0;
    std::string m_Name; // Useful for debugging
    std::unordered_map<std::string, int> m_UniformLocationCache;
};