#include "shader.hpp"

Shader::Shader(const char* vert_sh_dir, const char* frag_sh_dir){
    // read and compile shaders
    uint32_t vertShaderID = ReadCompileShader(GL_VERTEX_SHADER, vert_sh_dir);
    uint32_t fragShaderID = ReadCompileShader(GL_FRAGMENT_SHADER, frag_sh_dir);

    if (!vertShaderID){
        printf("[ERROR]: Shader object couldn't initalized\n");
        glDeleteShader(fragShaderID);
        return;
    } 
    else if (!fragShaderID){
        printf("[ERROR]: Shader object couldn't initalized\n");
        glDeleteShader(vertShaderID);
        return;
    }

    // create program
    m_ProgramID = glCreateProgram();
    glAttachShader(m_ProgramID, vertShaderID);
    glAttachShader(m_ProgramID, fragShaderID);
    glLinkProgram(m_ProgramID);
    
    // delete compiled shaders
    glDeleteShader(vertShaderID);
    glDeleteShader(fragShaderID);

    // error handling of program
    glValidateProgram(m_ProgramID);
    int32_t result;
    glGetProgramiv(m_ProgramID, GL_VALIDATE_STATUS, &result);
    if (result == GL_FALSE){
        int32_t size;
        glGetProgramiv(m_ProgramID, GL_INFO_LOG_LENGTH, &size);
        char * message = (char*)alloca(size * sizeof(char));
        glGetProgramInfoLog(m_ProgramID, size, &size, message);
        printf("[ERROR]: Program validation failed. %s \n", message);
        glDeleteProgram(m_ProgramID);
        return;
    }
}

Shader::~Shader(){
    glDeleteProgram(m_ProgramID);
}

void Shader::Use(){
    glUseProgram(m_ProgramID);
}

uint32_t Shader::GetProgramID(){
    return m_ProgramID;
}

int32_t Shader::GetUniformID(const char* uniform_name){
    auto it = m_UniformCache.find(uniform_name);
    if (it != m_UniformCache.end()){
        return it->second;
    }
    else {
        int32_t res = glGetUniformLocation(m_ProgramID, uniform_name);
        m_UniformCache[uniform_name] = res;
        return res;
    }
}

/* SETTING UNIFORMS */
bool Shader::SetUniform4f(const char* uniform_name, float x, float y, float z, float w){
    int32_t id = GetUniformID(uniform_name);
    glUniform4f(id, x, y, z, w);
    return true;
}
bool Shader::SetUniform4f(const int32_t &uniform_id, float x, float y, float z, float w){
    glUniform4f(uniform_id, x, y, z, w);
    return true;
}

bool Shader::SetUniform3f(const char* uniform_name, float x, float y, float z){
    int32_t id = GetUniformID(uniform_name);
    glUniform3f(id, x, y, z);
    return true;
}
bool Shader::SetUniform3f(const int32_t &uniform_id, float x, float y, float z){
    glUniform3f(uniform_id, x, y, z);
    return true;
}

bool Shader::SetUniform1f(const char* uniform_name, float x){
    int32_t id = GetUniformID(uniform_name);
    glUniform1f(id, x);
    return true;
}
bool Shader::SetUniform1f(const int32_t &uniform_id, float x){
    glUniform1f(uniform_id, x);
    return true;
}

bool Shader::SetUniform1i(const char* uniform_name, int32_t x){
    int32_t id = GetUniformID(uniform_name);
    glUniform1i(id, x);
    return true;
}
bool Shader::SetUniform1i(const int32_t &uniform_id, int32_t x){
    glUniform1i(uniform_id, x);
    return true;
}
///////////////////////////////////////////////////////////

uint32_t Shader::ReadCompileShader(uint32_t type, const char* &shader_dir){
    // read shader file
    char *shaderCode;
    if (!ReadFile(shader_dir, shaderCode)){
        return 0;
    }

    uint32_t id = glCreateShader(type);
    glShaderSource(id, 1, &shaderCode, NULL);
    glCompileShader(id);

    // delete the code's buffer
    delete[] shaderCode;

    // error handling of shader
    int32_t result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE){
        int32_t size;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &size);
        char * message = (char*)alloca(size * sizeof(char));
        glGetShaderInfoLog(id, size, &size, message);
        printf("[ERROR]::SHADER::COMPILE::");
        printf("%s", type == GL_VERTEX_SHADER ? "VERTEX_SHADER" : "FRAGMENT_SHADER");
        printf("\n%s", message);

        glDeleteShader(id);
        return 0;
    }

    return id;
}

bool Shader::ReadFile(const char * &file_dir, char* &buffer){
    std::ifstream file(file_dir, std::ios::in | std::ios::binary | std::ios::ate);

    // check the file
    if (!file.is_open()){
        printf("[ERROR]: An error occurred while opening the shader file \"%s\" \n", file_dir);
        return false;
    }

    uint32_t size = file.tellg(); 
    buffer = new char[size+1];
    // change pointer to the starting of the file
    file.seekg(0, std::ios::beg);
    file.read(buffer, size);
    file.close();
    // add null terminate character
    buffer[size] = '\0';
    return true;
}