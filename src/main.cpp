#include <stdio.h>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>

#include "shader.hpp"
#include "texture.hpp"

void frame_buffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window){
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }
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
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);

    // vertex buffer data
    float vertData[] = {
        // positions    // colors           // texture coords
        -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        0.5f, -0.5f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        0.5f, 0.5f,     0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f, 0.5f,    1.0f, 1.0f, 1.0f,   0.0f, 1.0f
    };

    // index buffer data
    uint16_t indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    uint32_t textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    // set the texture wrapping/filtering options (on the currently bounded texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture

    int32_t width, height, nCChannels;
    uint8_t * data = stbi_load("res/textures/container.jpg", &width, &height, &nCChannels, 0);
    if (data){
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // activate texture unit and bind the texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
    stbi_image_free(data);


    //
    uint32_t VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    uint32_t VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertData), vertData, GL_STATIC_DRAW);

    uint32_t EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // shaders
    Shader program_1("res/shaders/vertex.shader", "res/shaders/frag.shader");
    program_1.SetUniform1i("u_Texture", 0);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(2*sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(5*sizeof(float)));

    // render loop
    while(!glfwWindowShouldClose(window)){
        // input
        processInput(window);
        
        // clear buffer
        glClear(GL_COLOR_BUFFER_BIT);

        // activate the shader
        program_1.Use();

        // update color information
        float timeValue = glfwGetTime();
        float sin_wave = sin(timeValue) / 2.0f + 0.5f;  // sin range is [-1, 1] what we need is [0, 1]
        program_1.SetUniform1f("u_Color", sin_wave);

        // rendering commands here
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);

        // check and call events and swap the buffers
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}