//#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "FileIO.hpp"

struct Vector2D {
    float x;
    float y;
};

struct Uint8Vector3D {
    uint8_t x;
    uint8_t y;
    uint8_t z;
};

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

static uint32_t CreateProgram(std::string &vertex_code, std::string &fragm_code){
    uint32_t program_ID = glCreateProgram();
    uint32_t vertx_sh_ID = CompileShader(GL_VERTEX_SHADER, vertex_code);
    uint32_t frag_sh_ID = CompileShader(GL_FRAGMENT_SHADER, fragm_code);

    // link shaders with program
    glAttachShader(program_ID, vertx_sh_ID);
    glAttachShader(program_ID, frag_sh_ID);
    glLinkProgram(program_ID);
    glValidateProgram(program_ID);

    // delete compiled shaders
    glDeleteShader(vertx_sh_ID);
    glDeleteShader(frag_sh_ID);

    return program_ID;
}

int main_old(void){
    GLFWwindow* window;
    // Initialize the library
    if (!glfwInit())
        return -1;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(800, 600, "BioSphere Evolution", NULL, NULL);
    if (!window){
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Vsync

    if (glewInit() != GLEW_OK){
        return -1;
    }

    Vector2D positions[] = {
        {-0.5f, -0.5f}, 
        {0.5f, -0.5f},
        {0.0f, 0.0f},
        {0.5f, 0.5f},
        {-0.5f, 0.5f},
    };

    Uint8Vector3D index_buff[] = {
        {0, 1, 2},
        {2, 1, 3},
        {0, 1, 4}
    };

    float quadVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f,  1.0f,
        1.0f,  1.0f,
    };

    uint32_t vertex_array_ID, buffer_array_ID, element_array_ID;
    glGenVertexArrays(1, &vertex_array_ID);
    glBindVertexArray(vertex_array_ID);

    // vertex array buffer
    // VertexBuffer vbo(5*sizeof(Vector2D), positions, GL_STATIC_DRAW);
    // IndexBuffer ibo(3*sizeof(Uint8Vector3D), index_buff, GL_STATIC_DRAW);

    // generate buffers
    glGenBuffers(1, &buffer_array_ID);
    glGenBuffers(1, &element_array_ID);
    
    // vertex array buffer
    glBindBuffer(GL_ARRAY_BUFFER, buffer_array_ID); // select the currently generated buffer
    glBufferData(GL_ARRAY_BUFFER, 4*2*sizeof(float), quadVertices, GL_STATIC_DRAW);

    // element array buffer (index buffer)
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_array_ID);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*sizeof(Uint8Vector3D), index_buff, GL_STATIC_DRAW);

    // enable index zero attribute and create the index zero attribute
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), 0);

    // loading shaders
    std::string vert_shader;
    std::string frag_shader;
    LoadFile("res/shaders/vertex.shader", vert_shader);
    LoadFile("res/shaders/fragment.shader", frag_shader);

    // create Program with shaders
    uint32_t program_ID = CreateProgram(vert_shader, frag_shader);
    glUseProgram(program_ID);

    // set uniforms
    // int32_t uniform_loc = glGetUniformLocation(program_ID, "u_Color");
    // glUniform4f(uniform_loc, 1.0f, 0.0f, 0.0f, 1.0f);

    // Get uniform locations
    GLint sphereCenterLoc = glGetUniformLocation(program_ID, "sphereCenter");
    GLint sphereRadiusLoc = glGetUniformLocation(program_ID, "sphereRadius");
    GLint cameraPosLoc = glGetUniformLocation(program_ID, "cameraPos");
    GLint lightPosLoc = glGetUniformLocation(program_ID, "lightPos");
    GLint sphereColorLoc = glGetUniformLocation(program_ID, "sphereColor");

    // Set uniform values
    glUniform3f(sphereCenterLoc, 0.0f, 0.0f, -2.0f); // Sphere center
    glUniform1f(sphereRadiusLoc, 1.0f);              // Sphere radius
    glUniform3f(cameraPosLoc, 0.0f, 0.0f, 0.0f);     // Camera position
    glUniform3f(lightPosLoc, 5.0f, 5.0f, -5.0f);     // Light position
    glUniform3f(sphereColorLoc, 0.1019f, 0.8509f, 1.0f);   // Sphere color

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)){
        // Render here
        glClear(GL_COLOR_BUFFER_BIT);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
        //glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_BYTE, 0);
        
        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // de-allocate buffers
    glDeleteVertexArrays(1, &vertex_array_ID);
    
    glfwTerminate();
    return 0;
}