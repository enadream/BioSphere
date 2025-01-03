#include <stdio.h>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>

// opengl math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "texture.hpp"
#include "camera.hpp"
#include "temp_data.hpp"
#include "mesh.hpp"

static glm::mat4 projection;
static Camera * m_MainCamera;

static float deltaTime;
static int m_Width = 800, m_Height = 600;
static double lastX, lastY;

void frame_buffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
    projection = glm::perspective(glm::radians(m_MainCamera->m_Fov), (float)width/height, 0.1f, 100.0f);
    m_Width = width;
    m_Height = height;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos){
    // printf("mouse location x: %lf, y: %lf\n", xpos, ypos);
    const float k_sensitivity = 0.1f;
    //static double lastX, lastY;

    float yawOffset = (xpos - lastX) * k_sensitivity;
    float pitchOffset = (lastY - ypos) * k_sensitivity; // reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;
    
    m_MainCamera->Rotate(yawOffset, pitchOffset);
}

void processInput(GLFWwindow* window){
    static bool esc_key_pressed = false;
    if (!esc_key_pressed && glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        //glfwSetWindowShouldClose(window, true);
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED){
            glfwSetCursorPosCallback(window, NULL);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwGetCursorPos(window, &lastX, &lastY);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetCursorPosCallback(window, mouse_callback);
        }
        esc_key_pressed = true;
        //printf("mouse location x: %lf, y: %lf\n", lastX, lastY);
    } else if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE){
        esc_key_pressed = false;
    }

    float cameraSpeed = 2.5f; // adjust accordingly

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
        cameraSpeed += 5.0f;
    } 
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        m_MainCamera->ProcessMovement(Camera_Movement::FORWARD, cameraSpeed, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        m_MainCamera->ProcessMovement(Camera_Movement::BACKWARD, cameraSpeed, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        m_MainCamera->ProcessMovement(Camera_Movement::LEFT, cameraSpeed, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        m_MainCamera->ProcessMovement(Camera_Movement::RIGHT, cameraSpeed, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
        m_MainCamera->ProcessMovement(Camera_Movement::UP, cameraSpeed, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
        m_MainCamera->ProcessMovement(Camera_Movement::DOWN, cameraSpeed, deltaTime);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    m_MainCamera->ProcessMouseScroll(yoffset);
    projection = glm::perspective(glm::radians(m_MainCamera->m_Fov), (float)m_Width/m_Height, 0.1f, 100.0f);
}

int main(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    GLFWwindow* window = glfwCreateWindow(m_Width, m_Height, "BioSphere Evolution", NULL, NULL);
    
    if (window == NULL){
        printf("[ERROR]: Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Vsync

    glfwGetCursorPos(window, &lastX, &lastY);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // init glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("[ERROR]: Failed to initialize GLAD\n");
        return -1;
    }

    Camera cam(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_MainCamera = &cam;

    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    printf("Maximum nr of vertex attributes supported: %i \n", nrAttributes);

    // set viewport coordinats first 2 value are lower left corner of the window
    glViewport(0, 0, 800, 600);
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);

    glfwSetFramebufferSizeCallback(window, frame_buffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // main vertex array object
    uint32_t VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    uint32_t VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_vertices1), s_vertices1, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));

    // uint32_t EBO;
    // glGenBuffers(1, &EBO);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

   

    // glEnableVertexAttribArray(2);
    // glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    
    //trans = glm::scale(trans, glm::vec3(0.5f, 0.5f, 0.5f));
    //program_1.SetUniformMatrix4fv("u_Transform", glm::value_ptr(trans));

    // LS -[ModelMatrix]> WS -[ViewMatrix]> VS -[ProjectionMatrix]> CS -> SS
    // Model Matrix
    // glm::mat4 model = glm::mat4(1.0f);
    // model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // View Matrix
    glm::mat4 view = glm::mat4(1.0f);
    view = cam.GetViewMatrix();

    // Projection Matrix
    projection = glm::mat4(1.0f);
    projection = glm::perspective(glm::radians(cam.m_Fov), (float)m_Width/m_Height, 0.1f, 100.0f);

    glEnable(GL_DEPTH_TEST);

    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f,  2.0f, -2.5f),
        glm::vec3( 1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

     // create vertex array object for cube
    uint32_t obj1_VAO;
    glGenVertexArrays(1, &obj1_VAO);
    glBindVertexArray(obj1_VAO);

    // creating vertex array buffer
    uint32_t cube_VBO;
    glGenBuffers(1, &cube_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, cube_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_vertices2), s_vertices2, GL_STATIC_DRAW);
 
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));

    // create vertex array object for light cube
    uint32_t light_VAO;
    glGenVertexArrays(1, &light_VAO);
    glBindVertexArray(light_VAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));

    // light information
    glm::vec3 lightDir(0.0f, -1.0f, 0.0f);
    glm::vec3 lightColor(1.0f);

    glm::vec3 pointLightPositions[] = {
	    glm::vec3( 0.7f,  0.2f,  2.0f),
	    glm::vec3( 2.3f, -3.3f, -4.0f),
	    glm::vec3(-4.0f,  2.0f, -12.0f),
	    glm::vec3( 0.0f,  0.0f, -3.0f)
    };

    // shaders
    Shader program_1("res/shaders/vertex.glsl", "res/shaders/frag.glsl");
    program_1.Use();
    program_1.SetUniform1i("u_Texture", 0);
    program_1.SetUniform1i("u_Texture2", 1);
    program_1.SetUniform3fv("u_LightColor", lightColor);

    Shader lightingShader("res/shaders/cube.vert", "res/shaders/cube.frag");
    lightingShader.Use();
    lightingShader.SetUniform1i("u_material.DiffuseTexture", 2);
    lightingShader.SetUniform1i("u_material.SpecularTexture", 3);
    lightingShader.SetUniform1i("u_material.EmissionTexture", 4);
    lightingShader.SetUniform1f("u_material.Shininess", 64.0f);

    // directional light
    lightingShader.SetUniform3fv("u_DirLight.direction", lightDir);
    lightingShader.SetUniform3fv("u_DirLight.ambient", 0.05f * lightColor);
    lightingShader.SetUniform3fv("u_DirLight.diffuse",  0.4f * lightColor);
    lightingShader.SetUniform3fv("u_DirLight.specular", 0.5f * lightColor);
    
    // point lights
    lightingShader.SetUniform3fv("u_PointLights[0].position",  pointLightPositions[0]);
    lightingShader.SetUniform3fv("u_PointLights[0].ambient", 0.3f * lightColor);
    lightingShader.SetUniform3fv("u_PointLights[0].diffuse",  0.6f * lightColor);
    lightingShader.SetUniform3fv("u_PointLights[0].specular", lightColor);
    lightingShader.SetUniform1f("u_PointLights[0].constant",  1.0f);
    lightingShader.SetUniform1f("u_PointLights[0].linear",    0.09f);
    lightingShader.SetUniform1f("u_PointLights[0].quadratic", 0.032f);

    lightingShader.SetUniform3fv("u_PointLights[1].position",  pointLightPositions[1]);
    lightingShader.SetUniform3fv("u_PointLights[1].ambient", 0.05f * lightColor);
    lightingShader.SetUniform3fv("u_PointLights[1].diffuse",  0.8f * lightColor);
    lightingShader.SetUniform3fv("u_PointLights[1].specular", lightColor);
    lightingShader.SetUniform1f("u_PointLights[1].constant",  1.0f);
    lightingShader.SetUniform1f("u_PointLights[1].linear",    0.09f);
    lightingShader.SetUniform1f("u_PointLights[1].quadratic", 0.032f);

    lightingShader.SetUniform3fv("u_PointLights[2].position",  pointLightPositions[2]);
    lightingShader.SetUniform3fv("u_PointLights[2].ambient", 0.05f * lightColor);
    lightingShader.SetUniform3fv("u_PointLights[2].diffuse",  0.8f * lightColor);
    lightingShader.SetUniform3fv("u_PointLights[2].specular", lightColor);
    lightingShader.SetUniform1f("u_PointLights[2].constant",  1.0f);
    lightingShader.SetUniform1f("u_PointLights[2].linear",    0.09f);
    lightingShader.SetUniform1f("u_PointLights[2].quadratic", 0.032f);

    lightingShader.SetUniform3fv("u_PointLights[3].position",  pointLightPositions[3]);
    lightingShader.SetUniform3fv("u_PointLights[3].ambient", 0.05f * lightColor);
    lightingShader.SetUniform3fv("u_PointLights[3].diffuse",  0.8f * lightColor);
    lightingShader.SetUniform3fv("u_PointLights[3].specular", lightColor);
    lightingShader.SetUniform1f("u_PointLights[3].constant",  1.0f);
    lightingShader.SetUniform1f("u_PointLights[3].linear",    0.09f);
    lightingShader.SetUniform1f("u_PointLights[3].quadratic", 0.032f);
    
    // spot light
    lightingShader.SetUniform3fv("u_SpotLight.position", cam.m_Position);
    lightingShader.SetUniform3fv("u_SpotLight.direction", cam.m_Front);
    lightingShader.SetUniform3fv("u_SpotLight.ambient", 0.0f, 0.0f, 0.0f);
    lightingShader.SetUniform3fv("u_SpotLight.diffuse", 1.0f, 0.0f, 0.0f);
    lightingShader.SetUniform3fv("u_SpotLight.specular", 1.0f, 0.0f, 0.0f);
    lightingShader.SetUniform1f("u_SpotLight.constant", 1.0f);
    lightingShader.SetUniform1f("u_SpotLight.linear", 0.09f);
    lightingShader.SetUniform1f("u_SpotLight.quadratic", 0.032f);
    lightingShader.SetUniform1f("u_SpotLight.cutOff", glm::cos(glm::radians(12.5f)));
    lightingShader.SetUniform1f("u_SpotLight.outerCutOff", glm::cos(glm::radians(15.0f))); 

    Shader lightCubeShader("res/shaders/cube.vert", "res/shaders/light.frag");
    lightCubeShader.Use();
    lightCubeShader.SetUniform3fv("u_LightColor", lightColor);

    Texture texture1("res/textures/container.jpg");
    Texture texture2("res/textures/awesomeface.png");
    Texture texture3("res/textures/container2.png");
    Texture texture4("res/textures/container2_specular.png");
    Texture texture5("res/textures/matrix_emission.png");

    texture1.BindTo(0);
    texture2.BindTo(1);
    texture3.BindTo(2);
    texture4.BindTo(3);
    texture5.BindTo(4);

    // render loop
    while(!glfwWindowShouldClose(window)){
        // clear buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // delta time calculation
        static float lastFrameTime;
        deltaTime = glfwGetTime() - lastFrameTime;
        lastFrameTime = glfwGetTime();

        // input
        processInput(window);

        // update view matrix and projection matrix
        view = cam.GetViewMatrix();

        // // activate the shader
        // program_1.Use();
        // draw lighting cube objects 
        // program_1.SetUniformMatrix4fv("u_Projection", projection);
        // program_1.SetUniformMatrix4fv("u_View", view);
        // // draw 10 cubes
        // glBindVertexArray(VAO);
        // for (uint32_t i = 0; i < 1; i++){
        //     // calculate normal matrix
        //     glm::mat4 model1 = glm::mat4(1.0f);
        //     model1 = glm::translate(model1, cubePositions[i]);
        //     float angle = 20.0f * i;
        //     model1 = glm::rotate(model1, glm::radians((float)glfwGetTime() * angle / 50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
        //     program_1.SetUniformMatrix4fv("u_Model", model1);

        //     glm::mat3 matNormal = glm::mat3(matNormal);
        //     matNormal = glm::transpose(glm::inverse(matNormal));
        //     program_1.SetUniformMatrix3fv("u_Normal", matNormal);

        //     program_1.SetUniform3fv("u_CameraPos", cam.m_Position);
        //     glDrawArrays(GL_TRIANGLES, 0, 36);
        // }
        

        // draw lighting objects
        lightingShader.Use();
        lightingShader.SetUniformMatrix4fv("u_Projection", projection);
        lightingShader.SetUniformMatrix4fv("u_View", view);
        lightingShader.SetUniform3fv("u_CameraPos", cam.m_Position);

        glBindVertexArray(obj1_VAO);

        // update spotlight properties
        lightingShader.SetUniform3fv("u_SpotLight.position", cam.m_Position);
        lightingShader.SetUniform3fv("u_SpotLight.direction", cam.m_Front);

        for (int i = 0; i < 10; i++){
            // calculate the model matrix for each object and pass it to shader before drawing
            glm::mat4 model2 = glm::mat4(1.0f);
            model2 = glm::translate(model2, cubePositions[i]);
            float angle = 20.0f * i;
            model2 = glm::rotate(model2, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

            lightingShader.SetUniformMatrix4fv("u_Model", model2);
            // render the cube
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // draw light objects
        lightCubeShader.Use();
        lightCubeShader.SetUniformMatrix4fv("u_Projection", projection);
        lightCubeShader.SetUniformMatrix4fv("u_View", view);
        // draw 4 object
        glBindVertexArray(light_VAO);
        for (int i = 0; i < 4; i++){
            // world transformation
            glm::mat4 model3 = glm::mat4(1.0f);
            model3 = glm::translate(model3, pointLightPositions[i]);
            model3 = glm::scale(model3, glm::vec3(0.2f));
            lightCubeShader.SetUniformMatrix4fv("u_Model", model3);
            
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        

        glfwSwapBuffers(window);
        // check and call events and swap the buffers
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}