#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <unordered_map>
#include <string>
#include <fstream>

using std::unordered_map, std::string;

class Shader {
public: // functions
    Shader(const char * vert_sh_dir, const char * frag_sh_dir);
    Shader(const char * vert_sh_dir, const char * frag_sh_dir, const char * geo_sh_dir);
    ~Shader();
    
    // delete copy constructors
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    // move constructor
    Shader(Shader&& other) noexcept;

    inline uint32_t GetProgramID(){
        return m_ProgramID;
    }
    inline void Use(){
        glUseProgram(m_ProgramID);
    }

    int32_t GetUniformID(const string& uniform_name);

    void SetUniformMatrix4fv(const string& uniform_name, const glm::mat4 &value);
    void SetUniformMatrix4fv(const int32_t &uniform_id, const glm::mat4 &value);

    void SetUniformMatrix3fv(const string& uniform_name, const glm::mat3 &value);
    void SetUniformMatrix3fv(const int32_t &uniform_id, const glm::mat4 &value);

    void SetUniform4f(const string& uniform_name, float x, float y, float z, float w);
    void SetUniform4f(const int32_t &uniform_id, float x, float y, float z, float w);

    void SetUniform3f(const string& uniform_name, float x, float y, float z);
    void SetUniform3f(const int32_t &uniform_id, float x, float y, float z);

    void SetUniform1f(const string& uniform_name, float x);
    void SetUniform1f(const int32_t &uniform_id, float x);

    void SetUniform1i(const string& uniform_name, int32_t x);
    void SetUniform1i(const int32_t &uniform_id, int32_t x);

    void SetUniform3fv(const string& uniform_name, float x, float y, float z);
    void SetUniform3fv(const int32_t &uniform_id, float x, float y, float z);

    void SetUniform3fv(const string& uniform_name, const glm::vec3 &value);
    void SetUniform3fv(const int32_t &uniform_id, const glm::vec3 &value);
    
private: // variables
    uint32_t m_ProgramID;
    unordered_map<string, int32_t> m_UniformCache;

private: // functions

    bool readFile(const char * &file_dir, char* &buffer);
    uint32_t readCompileShader(uint32_t type, const char * &shader_dir);
};

#endif