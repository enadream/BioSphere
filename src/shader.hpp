#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/glad.h>
#include <unordered_map>
#include <fstream>

class Shader {
public:
    Shader(const char * vert_sh_dir, const char * frag_sh_dir);
    ~Shader();

    uint32_t GetProgramID();
    void Use();
    int32_t GetUniformID(const char* uniform_name);

    bool SetUniform4f(const char* uniform_name, float x, float y, float z, float w);
    bool SetUniform4f(const int32_t &uniform_id, float x, float y, float z, float w);

    bool SetUniform3f(const char* uniform_name, float x, float y, float z);
    bool SetUniform3f(const int32_t &uniform_id, float x, float y, float z);

    bool SetUniform1f(const char* uniform_name, float x);
    bool SetUniform1f(const int32_t &uniform_id, float x);

    bool SetUniform1i(const char* uniform_name, const int32_t x);
    bool SetUniform1i(const int32_t &uniform_id, const int32_t x);
private:
    uint32_t m_ProgramID;
    std::unordered_map<const char*, int32_t> m_UniformCache;

    bool ReadFile(const char * &file_dir, char* &buffer);
    uint32_t ReadCompileShader(uint32_t type, const char* &shader_dir);
};

#endif