#include "shader_program.hpp"

//#define DEBUG_SHADER

ShaderProgram::ShaderProgram(){
    // create program
    m_ProgramID = glCreateProgram();
}

ShaderProgram::ShaderProgram(const char* vert_sh_dir, const char* frag_sh_dir){
    // create program
    m_ProgramID = glCreateProgram();

    AttachShader(GL_VERTEX_SHADER, vert_sh_dir);
    AttachShader(GL_FRAGMENT_SHADER, frag_sh_dir);
    LinkShaders();
}

ShaderProgram::ShaderProgram(const char * vert_sh_dir, const char * frag_sh_dir, const char * geo_sh_dir) {
    // create program
    m_ProgramID = glCreateProgram();

    AttachShader(GL_VERTEX_SHADER, vert_sh_dir);
    AttachShader(GL_FRAGMENT_SHADER, frag_sh_dir);
    AttachShader(GL_GEOMETRY_SHADER, geo_sh_dir);

    LinkShaders();
}

ShaderProgram::~ShaderProgram(){
    glDeleteProgram(m_ProgramID);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : m_ProgramID(other.m_ProgramID),
    m_UniformCache(std::move(other.m_UniformCache)), m_ShaderIDs(std::move(other.m_ShaderIDs)){
    other.m_ProgramID = 0;
}

bool ShaderProgram::AttachShader(uint32_t type, const char *shader_dir){
    // read and compile shaders
    uint32_t shaderID = readCompileShader(type, shader_dir);

    if (!shaderID){
        printf("[ERROR]: Shader attachment failed. FilePath: \"%s\"\n", shader_dir);
        return false;
    }

    glAttachShader(m_ProgramID, shaderID);
    m_ShaderIDs.emplace_back(shaderID);
    return true;
}
bool ShaderProgram::LinkShaders() {
    // linking
    glLinkProgram(m_ProgramID);

    // delete compiled shaders
    for (uint32_t i = 0; i < m_ShaderIDs.size(); i++){
        glDeleteShader(m_ShaderIDs[i]);
    }
    m_ShaderIDs.clear();

    // error handling of program
    glValidateProgram(m_ProgramID);
    
    int32_t result;
    glGetProgramiv(m_ProgramID, GL_VALIDATE_STATUS, &result);
    if (result == GL_FALSE){
        int32_t size;
        glGetProgramiv(m_ProgramID, GL_INFO_LOG_LENGTH, &size);
        char * message = (char*)alloca(size * sizeof(char));
        glGetProgramInfoLog(m_ProgramID, size, &size, message);
        printf("[ERROR]: Program validation failed.\n");
        if (size > 0){
            printf("%s", message);
        }
        glDeleteProgram(m_ProgramID);
        m_ProgramID = 0;
        return false;
    }

    return true;
}


int32_t ShaderProgram::GetUniformID(const string& uniform_name){
    auto it = m_UniformCache.find(uniform_name);
    if (it != m_UniformCache.end()){
        return it->second;
    }
    else {
        int32_t res = glGetUniformLocation(m_ProgramID, uniform_name.c_str());
#ifdef DEBUG_SHADER
        if (res == -1){
            printf("[WARNING]: \"%s\" uniform couldn't found.\n", uniform_name.c_str());
        }
#endif
        m_UniformCache[uniform_name] = res;
        return res;
    }
}

/* SETTING UNIFORMS */

///////////////////////////////////////////////////////////

uint32_t ShaderProgram::readCompileShader(uint32_t type, const char * &shader_dir){
    // read shader file
    char *shaderCode;
    if (!readFile(shader_dir, shaderCode)){
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
        
        string shaderName;
        switch (type)
        {
        case GL_VERTEX_SHADER:
            shaderName = "VERTEX_SHADER";
            break;
        case GL_FRAGMENT_SHADER:
            shaderName = "FRAGMENT_SHADER";
            break;
        case GL_GEOMETRY_SHADER:
            shaderName = "GEOMETRY_SHADER";
            break;
        case GL_COMPUTE_SHADER:
            shaderName = "COMPUTE_SHADER";
            break;
        }
        printf("[ERROR]::SHADER::COMPILE::%s\n%s", shaderName.c_str(), message);

        // delete unsuccessfull shader
        glDeleteShader(id);
        return 0;
    }

    return id;
}

bool ShaderProgram::readFile(const char * &file_dir, char* &buffer){
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