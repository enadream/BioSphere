#include "Shader.cpp"

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

inline void Shader::Use(){
    glUseProgram(m_ProgramID);
}

inline int32_t Shader::GetUniformID(const std::string &uniform_name){
    if (m_UniformCache.count(uniform_name) > 0){
        return m_UniformCache[uniform_name];
    }
    else {
        int32_t res = glGetUniformLocation(m_ProgramID, uniform_name.c_str());
        m_UniformCache[uniform_name] = res;
        return res;
    }
}

/* SETTING UNIFORMS */
inline bool Shader::SetUniform4f(const std::string &uniform_name, float &x, float &y, float &z, float &w){
    int32_t id = GetUniformID(uniform_name);
    glUniform4f(id, x, y, z, w);
    return true;
}
inline bool Shader::SetUniform4f(const int32_t &uniform_id, float &x, float &y, float &z, float &w){
    glUniform4f(uniform_id, x, y, z, w);
    return true;
}

inline bool Shader::SetUniform3f(const std::string &uniform_name, float &x, float &y, float &z){
    int32_t id = GetUniformID(uniform_name);
    glUniform3f(id, x, y, z);
    return true;
}
inline bool Shader::SetUniform3f(const int32_t &uniform_id, float &x, float &y, float &z){
    glUniform3f(uniform_id, x, y, z);
    return true;
}

inline bool Shader::SetUniform1f(const std::string &uniform_name, float &x){
    int32_t id = GetUniformID(uniform_name);
    glUniform1f(id, x);
    return true;
}
inline bool Shader::SetUniform1f(const int32_t &uniform_id, float &x){
    glUniform1f(uniform_id, x);
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
        printf("%s" type == GL_VERTEX_SHADER ? "VERTEX_SHADER" : "FRAGMENT_SHADER");
        printf("\n%s\n", message);

        glDeleteShader(id);
        return 0;
    }

    return id;
}

bool Shader::ReadFile(const char * &file_dir, char* & out_file){
    std::ifstream file(file_dir, std::ios::in | std::ios::ate);

    // check the file
    if (!file.is_open()){
        printf("[ERROR]: An error occurred while opening the shader file \"%s\" \n", file_dir);
        return false;
    }

    std::streampos endPos = file.tellg();
    uint64_t size = endPos - file.seekg(0, std::ios::beg).tellg();
    out_file = new char[size];
    file.read(out_file, size);
    file.close;

    return true;
}