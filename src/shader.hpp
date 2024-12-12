#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/glad.h>
#include <unordered_map>
#include <string>
#include <fstream>

class Shader {
public:
    Shader(const char * vert_sh_dir, const char * frag_sh_dir);
    ~Shader();

    inline void Use();
    inline int32_t GetUniformID(const std::string &uniform_name);

    inline bool SetUniform4f(const std::string &uniform_name, float &x, float &y, float &z, float &w);
    inline bool SetUniform4f(const int32_t &uniform_id, float &x, float &y, float &z, float &w);

    inline bool SetUniform3f(const std::string &uniform_name, float &x, float &y, float &z);
    inline bool SetUniform3f(const int32_t &uniform_id, float &x, float &y, float &z);

    inline bool SetUniform1f(const std::string &uniform_name, float &x);
    inline bool SetUniform1f(const int32_t &uniform_id, float &x);
private:
    uint32_t m_ProgramID;
    std::unordered_map<std::string, int32_t> m_UniformCache;

    bool ReadFile(const char * &file_dir, char* & out_file);
    uint32_t ReadCompileShader(uint32_t type, const char* &shader_dir);
};

#endif