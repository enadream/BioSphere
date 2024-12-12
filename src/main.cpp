#include <stdio.h>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "FileIO.hpp"
#include <math.h>

void frame_buffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window){
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }
}

static uint32_t CompileShader(uint32_t type, const std::string &source){
    uint32_t id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    // TODO: Error Handling
    int32_t result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE){
        int32_t log_size;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_size);
        char * message = (char*)alloca(log_size*sizeof(char));
        glGetShaderInfoLog(id, log_size, &log_size, message);
        printf("[ERROR]: ");
        printf(type == GL_VERTEX_SHADER ? "Vertex Shader\n" : "Fragment Shader\n");
        printf("%s \n", message);
        glDeleteShader(id);
        return 0;
    }
    
    return id;
}

static uint32_t CreateProgram(const char *vertex_dir, const char *fragm_dir){
    std::string vertex_code, fragm_code;
    LoadFile(vertex_dir, vertex_code);
    LoadFile(fragm_dir, fragm_code);

    uint32_t program_ID = glCreateProgram();
    uint32_t vertx_sh_ID = CompileShader(GL_VERTEX_SHADER, vertex_code);
    uint32_t frag_sh_ID = CompileShader(GL_FRAGMENT_SHADER, fragm_code);

    // link shaders with program
    glAttachShader(program_ID, vertx_sh_ID);
    glAttachShader(program_ID, frag_sh_ID);
    glLinkProgram(program_ID);
    glValidateProgram(program_ID);

    // TODO: Error Handling
    int32_t result;
    glGetProgramiv(program_ID, GL_VALIDATE_STATUS, &result);
    
    if (result == GL_FALSE){
        int32_t log_size;
        glGetProgramiv(program_ID, GL_INFO_LOG_LENGTH, &log_size);
        char * message = (char*)alloca(log_size*sizeof(char));
        glGetProgramInfoLog(program_ID, log_size, &log_size, message);
        printf("[ERROR]: Program validation failed. %s \n", message);
        glDeleteProgram(program_ID);
        return 0;
    }

    // delete compiled shaders
    glDeleteShader(vertx_sh_ID);
    glDeleteShader(frag_sh_ID);

    return program_ID;
}

int main(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); for macOS
    

    GLFWwindow* window = glfwCreateWindow(800, 600, "BioSphere Evolution", NULL, NULL);
    if (window == NULL){
        printf("[ERROR]: Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Vsync

    // init glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("[ERROR]: Failed to initialize GLAD\n");
        return -1;
    }

    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    printf("Maximum nr of vertex attributes supported: %i \n", nrAttributes);


    // set viewport coordinats first 2 value are lower left corner of the window
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, frame_buffer_size_callback);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    // vertex buffer data
    float vertices[] = {
        -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 1.0f, 1.0f, 1.0f
    };

    // index buffer data
    uint16_t indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    uint32_t VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    uint32_t VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    uint32_t EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // shaders
    uint32_t program_1 = CreateProgram("res/shaders/vertex.shader", "res/shaders/frag.shader");
    uint32_t program_2 = CreateProgram("res/shaders/vertex.shader", "res/shaders/frag2.shader");

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(2*sizeof(float)));

    // render loop
    while(!glfwWindowShouldClose(window)){
        // input
        processInput(window);
        
        // clear buffer
        glClear(GL_COLOR_BUFFER_BIT);

        // activate the shader
        glUseProgram(program_1);
        int32_t uniform_color_loc = glGetUniformLocation(program_1, "u_Color");

        // update color information
        float timeValue = glfwGetTime();
        float sin_wave = sin(timeValue) / 2.0f + 0.5f;  // sin range is [-1, 1] what we need is [0, 1]
        glUniform1f(uniform_color_loc, sin_wave);

        // rendering commands here
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);

        // check and call events and swap the buffers
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}