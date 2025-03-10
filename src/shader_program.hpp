#ifndef SHADER_PROGRAM_HPP
#define SHADER_PROGRAM_HPP

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>

using std::unordered_map, std::string;

class ShaderProgram {
public: // functions
    ShaderProgram();
    ShaderProgram(const char * vert_sh_dir, const char * frag_sh_dir);
    ShaderProgram(const char * vert_sh_dir, const char * frag_sh_dir, const char * geo_sh_dir);
    ~ShaderProgram();
    
    // delete copy constructors
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    // move constructor
    ShaderProgram(ShaderProgram&& other) noexcept;

    bool AttachShader(uint32_t type, const char * shader_dir);
    bool LinkShaders();

    inline uint32_t GetProgramID(){
        return m_ProgramID;
    }
    inline void Use(){
        glUseProgram(m_ProgramID);
    }
    inline void Unuse(){
        glUseProgram(0);
    }

    // Uniform setters
    int32_t GetUniformID(const string& uniform_name);
    
private: // variables
    uint32_t m_ProgramID;
    std::vector<uint32_t> m_ShaderIDs;
    unordered_map<string, int32_t> m_UniformCache;

private: // functions
    bool readFile(const char * &file_dir, char* &buffer);
    uint32_t readCompileShader(uint32_t type, const char * &shader_dir);

public: //////////////////////////// Uniform Setters ////////////////////////////
inline void SetUniformMatrix4fv(const string& uniform_name, const glm::mat4 &value){
    int32_t id = GetUniformID(uniform_name);
    glUniformMatrix4fv(id, 1, GL_FALSE, glm::value_ptr(value));
}
inline void SetUniformMatrix4fv(const int32_t &uniform_id, const glm::mat4 &value){
    glUniformMatrix4fv(uniform_id, 1, GL_FALSE, glm::value_ptr(value));
}

inline void SetUniformMatrix3fv(const string& uniform_name, const glm::mat3 &value){
    int32_t id = GetUniformID(uniform_name);
    glUniformMatrix3fv(id, 1, GL_FALSE, glm::value_ptr(value));
}
inline void SetUniformMatrix3fv(const int32_t &uniform_id, const glm::mat4 &value){
    glUniformMatrix3fv(uniform_id, 1, GL_FALSE, glm::value_ptr(value));
}

inline void SetUniform4f(const string& uniform_name, float x, float y, float z, float w){
    int32_t id = GetUniformID(uniform_name);
    glUniform4f(id, x, y, z, w);
}
inline void SetUniform4f(const int32_t &uniform_id, float x, float y, float z, float w){
    glUniform4f(uniform_id, x, y, z, w);
}

inline void SetUniform3f(const string& uniform_name, float x, float y, float z){
    int32_t id = GetUniformID(uniform_name);
    glUniform3f(id, x, y, z);
}
inline void SetUniform3f(const int32_t &uniform_id, float x, float y, float z){
    glUniform3f(uniform_id, x, y, z);
}

inline void SetUniform1f(const string& uniform_name, float x){
    int32_t id = GetUniformID(uniform_name);
    glUniform1f(id, x);
}
inline void SetUniform1f(const int32_t &uniform_id, float x){
    glUniform1f(uniform_id, x);
}


inline void SetUniform1ui(const string& uniform_name, uint32_t x){
    int32_t id = GetUniformID(uniform_name);
    glUniform1ui(id, x);
}
inline void SetUniform1ui(const int32_t &uniform_id, uint32_t x){
    glUniform1ui(uniform_id, x);
}

inline void SetUniform1i(const string& uniform_name, int32_t x){
    int32_t id = GetUniformID(uniform_name);
    glUniform1i(id, x);
}
inline void SetUniform1i(const int32_t &uniform_id, int32_t x){
    glUniform1i(uniform_id, x);
}
inline void SetUniform2iv(const string& uniform_name, glm::ivec2 vec2d){
    int32_t id = GetUniformID(uniform_name);
    glUniform2iv(id, 1, glm::value_ptr(vec2d));
}
inline void SetUniform2iv(const int32_t &uniform_id, glm::ivec2 vec2d){
    glUniform2iv(uniform_id, 1, glm::value_ptr(vec2d));
}
inline void SetUniform2fv(const string& uniform_name, glm::vec2 vec2d){
    int32_t id = GetUniformID(uniform_name);
    glUniform2fv(id, 1, glm::value_ptr(vec2d));
}
inline void SetUniform2fv(const int32_t &uniform_id, glm::vec2 vec2d){
    glUniform2fv(uniform_id, 1, glm::value_ptr(vec2d));
}

inline void SetUniform3fv(const string& uniform_name, float x, float y, float z){
    int32_t id = GetUniformID(uniform_name);
    float vec3[3] = {x, y, z};
    glUniform3fv(id, 1, vec3);
}
inline void SetUniform3fv(const int32_t &uniform_id, float x, float y, float z){
    float vec3[3] = {x, y, z};
    glUniform3fv(uniform_id, 1, vec3);
}
inline void SetUniform3fv(const string& uniform_name, const glm::vec3 &value, int count = 1){
    int32_t id = GetUniformID(uniform_name);
    glUniform3fv(id, count, glm::value_ptr(value));
}
inline void SetUniform3fv(const int32_t &uniform_id, const glm::vec3 &value, int count = 1){
    glUniform3fv(uniform_id, count, glm::value_ptr(value));
}
inline void SetUniform4fv(const string& uniform_name, const glm::vec4 &value, int count = 1){
    int32_t id = GetUniformID(uniform_name);
    glUniform4fv(id, count, glm::value_ptr(value));
}
inline void SetUniform4fv(const int32_t &uniform_id, const glm::vec4 &value, int count = 1){
    glUniform4fv(uniform_id, count, glm::value_ptr(value));
}
};

#endif