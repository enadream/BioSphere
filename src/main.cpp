#include <stdio.h>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <vector>

#include <iostream>
#include <sstream>

// opengl math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>

#include "texture.hpp"
#include "camera.hpp"
#include "temp_data.hpp"
#include "shader.hpp"
#include "vertex_array.hpp"
#include "bound_box.hpp"
#include "chunk.hpp"
#include "draw_array_command.hpp"

static Camera * m_MainCamera;

static float deltaTime;
static double lastX, lastY;

constexpr float sphereRadius = 0.5f;

#include "debug_func.hpp"

static glm::mat4 lookToCamera(const glm::vec3 &quad_pos, float radius, float &scale_factor);
void frame_buffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
float get_block_height(float x, float z);
float get_block_height_perlin(glm::vec3 &block_pos, float radius, int floor);
float generateHeight(const glm::vec3& position);
float terrain_height(const glm::vec3& position);

int main(){
    Camera cam(glm::vec3(0.30f, 80.05f, 0.30f), 1.0f, 10000.0f, 800, 600);
    m_MainCamera = &cam;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 1); // MSAA
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    GLFWwindow* window = glfwCreateWindow(cam.GetWidth(), cam.GetHeight(), "BioSphere Evolution", NULL, NULL);
    
    if (window == NULL){
        printf("[ERROR]: Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // Vsync

    glfwGetCursorPos(window, &lastX, &lastY);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    

    // init glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("[ERROR]: Failed to initialize GLAD\n");
        return -1;
    }

    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    printf("Maximum nr of vertex attributes supported: %i \n", nrAttributes);

    glDisable(GL_MULTISAMPLE);
    //glEnable(GL_FRAMEBUFFER_SRGB); // gamma correction
    // set viewport coordinats first 2 value are lower left corner of the window
    glViewport(0, 0, 800, 600);
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glEnable(GL_CULL_FACE);
    // blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetFramebufferSizeCallback(window, frame_buffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);


    //////////////// CHUNKS HOLDER ////////////////////////////////////////////////////////////////////////////////////////////////
    ChunkHolder chunkHolder(100, sphereRadius); // total amount of chunk is x*x
    cam.SetPositionX(chunkHolder.GetVertChunkAmount()*CHUNK_SIZE*sphereRadius / 2.0f);
    cam.SetPositionY(150.0f);
    cam.SetPositionZ(chunkHolder.GetVertChunkAmount()*CHUNK_SIZE*sphereRadius / 2.0f);

    float sphereVertices[] = {
        -1, -1,
        1, -1,
        -1, 1,
        1, 1
    };

    // load instanced arrays
    VertexBuffer sphereVBO, instanceVBO;
    sphereVBO.GenVertexBuffer(sizeof(sphereVertices), sphereVertices, 4, GL_STATIC_DRAW);
    instanceVBO.GenVertexBuffer(chunkHolder.GetTotalNumOfSpheres()*sizeof(glm::vec3), (void*)0, chunkHolder.GetTotalNumOfSpheres(), GL_STATIC_DRAW);
    
    instanceVBO.Bind();
    uint32_t sphereOffsetBytes = 0;
    for (uint32_t i = 0; i < chunkHolder.chunks.size(); i++){
        glBufferSubData(GL_ARRAY_BUFFER, sphereOffsetBytes, chunkHolder.chunks[i].m_Positions.size() * sizeof(glm::vec3),
            chunkHolder.chunks[i].m_Positions.data());
        sphereOffsetBytes += chunkHolder.chunks[i].m_Positions.size() * sizeof(glm::vec3);
    }
    instanceVBO.Unbind();

    uint32_t sphereVAO;
    glGenVertexArrays(1, &sphereVAO);
    glBindVertexArray(sphereVAO);
    sphereVBO.Bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    instanceVBO.Bind();
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(1, 1);
    sphereVBO.Unbind();
    instanceVBO.Unbind();
    glBindVertexArray(0);

    printf("Total number of spheres: %u\n", chunkHolder.GetTotalNumOfSpheres());

    // create command array
    std::vector<DrawArraysIndirectCommand> drawCommands;
    drawCommands.reserve(chunkHolder.GetTotalChunkAmount());

    // Sphere TEXTURES
    std::vector<string> spFaces = 
    {
        "res/textures/dirt_block/grass_cartoon_256.jpg",
        "res/textures/dirt_block/grass_cartoon_256.jpg",
        "res/textures/dirt_block/grass_cartoon_256.jpg",
        "res/textures/dirt_block/grass_cartoon_256.jpg",
        "res/textures/dirt_block/grass_cartoon_256.jpg",
        "res/textures/dirt_block/grass_cartoon_256.jpg"
    };

    CubeTexture sphereTex;
    sphereTex.LoadCubeMap(spFaces);

    // SKYBOX
    VertexArray skybox;
    skybox.GenVertexBuffer(sizeof(s_skyboxVertices), (void*)s_skyboxVertices, 36, GL_STATIC_DRAW);
    skybox.InsertLayout(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);

    // SKYBOX TEXTURES
    std::vector<string> faces = 
    {
        "res/textures/skybox/right.jpg",
        "res/textures/skybox/left.jpg",
        "res/textures/skybox/top.jpg",
        "res/textures/skybox/bottom.jpg",
        "res/textures/skybox/front.jpg",
        "res/textures/skybox/back.jpg"
    };

    CubeTexture skyboxTex;
    skyboxTex.LoadCubeMap(faces);

    // light information
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f));
    glm::vec3 lightColor(1.0f);

    // shaders
    Shader skyboxShader("res/shaders/skybox.vert", "res/shaders/skybox.frag");
    Shader sphereShader("res/shaders/sphere2.vert", "res/shaders/sphere.frag"); // , "res/shaders/sphere.geom"
    sphereShader.Use();
    // directional light
    sphereShader.SetUniform3fv("u_DirLight.direction", glm::normalize(lightDir));
    sphereShader.SetUniform3fv("u_DirLight.ambient", 0.5f * glm::vec3(1.0f));
    sphereShader.SetUniform3fv("u_DirLight.diffuse",  0.5f * glm::vec3(1.0f));
    sphereShader.SetUniform3fv("u_DirLight.specular", 0.5f * glm::vec3(1.0f));

    sphereShader.SetUniform3fv("u_PointLight.position",  glm::vec3(0.0f, 5.0f, 0.0f));
    sphereShader.SetUniform3fv("u_PointLight.ambient", 0.05f * lightColor);
    sphereShader.SetUniform3fv("u_PointLight.diffuse",  0.8f * lightColor);
    sphereShader.SetUniform3fv("u_PointLight.specular", lightColor);
    sphereShader.SetUniform1f("u_PointLight.constant",  1.0f);
    sphereShader.SetUniform1f("u_PointLight.linear",    0.09f);
    sphereShader.SetUniform1f("u_PointLight.quadratic", 0.032f);

    // set sphere information
    sphereShader.SetUniform1f("u_Radius", sphereRadius);
    sphereShader.SetUniform1f("u_FarDist", cam.GetFar());

    std::string title = "BioSphere Evolution : ";
    float frameUpate = 0.0;
    uint32_t frameCount = 0;

    VertexArray box;
    box.GenVertexBuffer(sizeof(s_cubeVertonly), s_cubeVertonly, 36, GL_STATIC_DRAW);
    box.InsertLayout(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);
    Shader boxShader("res/shaders/bound_box.vert", "res/shaders/bound_box.frag");

    // MATRICES
    glm::mat4 proj, view, projView;
    uint32_t test = 0;

    while(!glfwWindowShouldClose(window)){
        // clear buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // delta time calculation
        static float lastFrameTime;
        deltaTime = glfwGetTime() - lastFrameTime;
        lastFrameTime = glfwGetTime();

        // print fps
        if (frameUpate > 0.99999){ // every 0.25 sec update the fps counter
            title.resize(22);
            title += std::to_string(int(1.0/(frameUpate/frameCount))) + " fps / ";
            title += std::to_string((frameUpate/frameCount)*1000) + "ms";
            glfwSetWindowTitle(window, title.c_str());
            frameUpate = 0.0;
            frameCount = 0;
        }
        frameUpate += deltaTime;
        frameCount++;

        // input
        processInput(window);

        // update view matrix and projection matrix
        proj = cam.GetProjMatrix();
        view = cam.GetViewMatrix();
        projView = proj * view;

        // calculate view frustum
        cam.CalculateFrustum();

        
        // set uniform data of sphere shader
        sphereShader.Use();
        sphereShader.SetUniformMatrix4fv("u_ProjView", projView);
        sphereShader.SetUniform3fv("u_CameraPos", cam.GetPosition());
        //sphereShader.SetUniform3fv("u_DirLight.ambient", 0.3f * glm::vec3(1.0f));
        //sphereShader.SetUniform3fv("u_DirLight.diffuse",  0.5f * glm::vec3(1.0f));
        //sphereShader.SetUniform3fv("u_DirLight.specular", 0.5f * glm::vec3(1.0f));
        sphereShader.SetUniform3fv("u_Color", glm::vec3(0.5f, 0.8f, 1.0f));
        sphereShader.SetUniform1i("u_Texture", 1);
        sphereTex.BindTo(1);
        
        glBindVertexArray(sphereVAO);
        //glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instanceVBO.GetVertAmount());
        
        
        // for (uint32_t i = 0; i < 400000; i++){ // max 400 k frustum check is good
        //     if (chunkHolder.chunks[0].m_BoundBox.IsOnFrustum(cam.m_Frustum)){
        //         test += i*1;
        //     }
        // }
        
        // draw spheres
        for (uint32_t i = 0; i < chunkHolder.chunks.size(); i++){
            if (chunkHolder.chunks[i].m_BoundBox.IsOnFrustum(cam.m_Frustum)){
                glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, chunkHolder.chunks[i].m_Positions.size(),
                    chunkHolder.chunks[i].m_StartIndex);
                // boxShader.Use();
                // boxShader.SetUniformMatrix4fv("u_ProjView", projView);
                
                // glm::mat4 modelBoundBox = glm::mat4(1.0f);
                // modelBoundBox = glm::translate(modelBoundBox, cam.GetUp() + cam.GetFront() * 10.0f + cam.GetPosition());
                // modelBoundBox = glm::scale(modelBoundBox, 5.0f*glm::vec3(1.0f));
                // boxShader.SetUniformMatrix4fv("u_Model", modelBoundBox);
                // box.Bind();
                // glDrawArrays(GL_TRIANGLES, 0, box.m_VertBuffer.GetVertAmount());  
            }

            // boxShader.Use();
            // boxShader.SetUniformMatrix4fv("u_ProjView", projView);
            // glm::mat4 modelBoundBox = glm::mat4(1.0f);
            // modelBoundBox = glm::translate(modelBoundBox, chunkHolder.chunks[i].m_BoundBox.center);
            // modelBoundBox = glm::scale(modelBoundBox, chunkHolder.chunks[i].m_BoundBox.extents*2.0f);
            // boxShader.SetUniformMatrix4fv("u_Model", modelBoundBox);
            // box.Bind();
            // glDrawArrays(GL_TRIANGLES, 0, box.m_VertBuffer.GetVertAmount());  
        }
        // boxShader.Use();
        // boxShader.SetUniformMatrix4fv("u_ProjView", projView);
        
        // glm::mat4 modelBoundBox = glm::mat4(1.0f);
        // modelBoundBox = glm::translate(modelBoundBox, cam.m_Up + cam.m_Front * 10.0f + cam.m_Position);
        // modelBoundBox = glm::scale(modelBoundBox, 5.0f*glm::vec3(1.0f));
        // boxShader.SetUniformMatrix4fv("u_Model", modelBoundBox);
        // box.Bind();
        // glDrawArrays(GL_TRIANGLES, 0, box.m_VertBuffer.GetVertAmount());  
        

        // // draw skybox
        // //glDepthFunc(GL_LEQUAL);
        // skyboxShader.Use();
        // skyboxShader.SetUniformMatrix4fv("u_Projection", projection);
        // glm::mat4 skyView = glm::mat4(glm::mat3(view));
        // skyboxShader.SetUniformMatrix4fv("u_View", skyView);
        // skyboxShader.SetUniform1i("u_Skybox", 0);
        // skyboxTex.BindTo(0);
        // skybox.Bind();
        // glDrawArrays(GL_TRIANGLES, 0, skybox.m_VertBuffer.GetVertAmount());
        // //glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
        // check and call events and swap the buffers
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


// float generateHeight(const glm::vec3& position) {
//     constexpr float baseFrequency = 0.005f;
//     constexpr float baseAmplitude = 50.0f;

//     constexpr float detailFrequency = 0.05f;
//     constexpr float detailAmplitude = 0.0f;

//     // Generate base terrain (large-scale features)
//     float baseHeight = Simplex::fBm(glm::vec2(position.x, position.z) * baseFrequency, 4, 2.0f, 0.5f) * baseAmplitude;

//     // Generate details (small-scale noise)
//     float detailHeight = Simplex::fBm(glm::vec2(position.x, position.z) * detailFrequency, 3, 2.0f, 0.5f) * detailAmplitude;

//     // Combine base terrain and details
//     float height = baseHeight + detailHeight;

//     return height;
// }

// // Usage function
// float getHeight(float x, float z) {
//     return terrain(vec2(x, z));
// }

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

    float cameraSpeed = 10.0f; // adjust accordingly

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
        cameraSpeed += 50.0f;
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
        m_MainCamera->ProcessMovement(Camera_Movement::DOWN, cameraSpeed, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
        m_MainCamera->ProcessMovement(Camera_Movement::UP, cameraSpeed, deltaTime);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    m_MainCamera->ProcessMouseScroll(yoffset);
}

void frame_buffer_size_callback(GLFWwindow* window, int width, int height){
    if (width == 0 || height == 0) {
        // Window is minimized; skip rendering
        return;
    }
    glViewport(0, 0, width, height);
    m_MainCamera->SetWidthHeight(width, height);
}

// Function to convert glm::vec3 to string
std::string vec3ToString(const glm::vec3& vec) {
    std::ostringstream oss;
    oss << "vec3(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return oss.str();
}

// Function to convert glm::mat4 to string
std::string mat4ToString(const glm::mat4& mat) {
    std::ostringstream oss;
    oss << "mat4(\n";
    for (int i = 0; i < 4; ++i) {
        oss << "  (" << mat[i][0] << ", " << mat[i][1] << ", " << mat[i][2] << ", " << mat[i][3] << ")\n";
    }
    oss << ")";
    return oss.str();
}

// static glm::mat4 lookToCamera(const glm::vec3 &quad_pos, float radius, float &scale_factor){

//     // Calculate direction to the camera
//     glm::vec3 distVector = m_MainCamera->m_Position - quad_pos;
//     glm::vec3 direction = glm::normalize(distVector);

//     // Compute right and up vectors for the quad
//     glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0, 1.0, 0.0), direction)); // m_MainCamera->m_WorldUp
//     glm::vec3 up = glm::cross(direction, right);

//     // Create the rotation matrix
//     glm::mat4 rotation = glm::mat4(1.0f);
//     rotation[0] = glm::vec4(right, 0.0f);
//     rotation[1] = glm::vec4(up, 0.0f);
//     rotation[2] = glm::vec4(direction, 0.0f); // Negative to face the camera

//     // Create the translation matrix
//     glm::mat4 translation = glm::translate(glm::mat4(1.0f), quad_pos);

//     // scale
//     float distance = glm::length(distVector);
//     glm::mat4 scale;
    

//     if (distance > 1.1*radius){
//         scale_factor = distance / glm::sqrt((distance-radius)*(distance+radius));
//         scale = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor, scale_factor, scale_factor));
//     } else {
//         scale_factor = 1.0;
//         scale = glm::mat4(1.0f);
//     }

    
//     // Combine translation and rotation
//     glm::mat4 model = translation * rotation * scale;


//     // Debug output
//     // std::cout << "Direction: " << vec3ToString(direction) << std::endl;
//     // std::cout << "Model Matrix: " << mat4ToString(model) << std::endl;
    

//     return model;
// }