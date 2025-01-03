#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <unordered_map>
#include <fstream>

class Shader {
public:
    Shader(const char * vert_sh_dir, const char * frag_sh_dir);
    ~Shader();

    uint32_t GetProgramID();
    void Use();
    int32_t GetUniformID(const char* uniform_name);

    bool SetUniformMatrix4fv(const char* uniform_name, const glm::mat4 &value);
    bool SetUniformMatrix4fv(const int32_t &uniform_id, const glm::mat4 &value);

    bool SetUniformMatrix3fv(const char* uniform_name, const glm::mat3 &value);
    bool SetUniformMatrix3fv(const int32_t &uniform_id, const glm::mat4 &value);

    bool SetUniform4f(const char* uniform_name, float x, float y, float z, float w);
    bool SetUniform4f(const int32_t &uniform_id, float x, float y, float z, float w);

    bool SetUniform3f(const char* uniform_name, float x, float y, float z);
    bool SetUniform3f(const int32_t &uniform_id, float x, float y, float z);

    bool SetUniform1f(const char* uniform_name, float x);
    bool SetUniform1f(const int32_t &uniform_id, float x);

    bool SetUniform1i(const char* uniform_name, int32_t x);
    bool SetUniform1i(const int32_t &uniform_id, int32_t x);

    bool SetUniform3fv(const char* uniform_name, float x, float y, float z);
    bool SetUniform3fv(const int32_t &uniform_id, float x, float y, float z);

    bool SetUniform3fv(const char* uniform_name, const glm::vec3 &value);
    bool SetUniform3fv(const int32_t &uniform_id, const glm::vec3 &value);
    
private:
    uint32_t m_ProgramID;
    std::unordered_map<const char*, int32_t> m_UniformCache;

    bool readFile(const char * &file_dir, char* &buffer);
    uint32_t readCompileShader(uint32_t type, const char* &shader_dir);
};

#endif