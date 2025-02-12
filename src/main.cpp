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
#include "shader_program.hpp"
#include "vertex_array.hpp"
#include "bound_box.hpp"
#include "chunk.hpp"
#include "draw_array_command.hpp"
#include "gl_buffer.hpp"
#include "frame_buffer.hpp"

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
void print_limits();
std::string vec3ToString(const glm::vec3& vec);

int main(){
    Camera cam(glm::vec3(0.30f, 80.05f, 0.30f), 1.0f, 1000.0f, 800, 600);
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

    print_limits();

    glDisable(GL_MULTISAMPLE);
    //glEnable(GL_FRAMEBUFFER_SRGB); // gamma correction
    // set viewport coordinats first 2 value are lower left corner of the window
    glViewport(0, 0, 800, 600);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // Depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glEnable(GL_CULL_FACE);
    // blending
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetFramebufferSizeCallback(window, frame_buffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);


    //////////////// CHUNKS HOLDER ////////////////////////////////////////////////////////////////////////////////////////////////
    ChunkHolder chunkHolder(100 * CHUNK_SIZE, sphereRadius); // total amount of chunk is x*x
    cam.SetPositionX(0.05 + chunkHolder.GetWidth()*sphereRadius / 2.0f);
    cam.SetPositionY(150.0f);
    cam.SetPositionZ(0.07 + chunkHolder.GetWidth()*sphereRadius / 2.0f);

    printf("Total number of spheres: %u\n", chunkHolder.spheres.size());

    VertexArray spheresVAO;
    spheresVAO.GenVertexBuffer(chunkHolder.spheres.size()*sizeof(Sphere), 
        chunkHolder.spheres.data(), chunkHolder.spheres.size(), GL_STATIC_DRAW);
    spheresVAO.InsertLayoutI(0, 3, GL_INT, sizeof(Sphere), 0);
    spheresVAO.InsertLayout(1, 1, GL_FLOAT, GL_FALSE, sizeof(Sphere), offsetof(Sphere, radius));

    ShaderProgram sphereProgram("res/shaders/sphere.vert", "res/shaders/sphere.frag", "res/shaders/sphere.geom");

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
    skybox.InsertLayout(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);

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
    ShaderProgram skyboxShader("res/shaders/skybox.vert", "res/shaders/skybox.frag");
    ShaderProgram sphereShader("res/shaders/sphere3.vert", "res/shaders/sphere.frag"); // , "res/shaders/sphere.geom"
    sphereShader.Use();
    // directional light
    sphereShader.SetUniform3fv("u_DirLight.direction", glm::normalize(lightDir));
    sphereShader.SetUniform3fv("u_DirLight.ambient", 0.5f * glm::vec3(1.0f));
    sphereShader.SetUniform3fv("u_DirLight.diffuse",  0.5f * glm::vec3(1.0f));
    sphereShader.SetUniform3fv("u_DirLight.specular", 0.5f * glm::vec3(1.0f));

    // sphereShader.SetUniform3fv("u_PointLight.position",  glm::vec3(0.0f, 5.0f, 0.0f));
    // sphereShader.SetUniform3fv("u_PointLight.ambient", 0.05f * lightColor);
    // sphereShader.SetUniform3fv("u_PointLight.diffuse",  0.8f * lightColor);
    // sphereShader.SetUniform3fv("u_PointLight.specular", lightColor);
    // sphereShader.SetUniform1f("u_PointLight.constant",  1.0f);
    // sphereShader.SetUniform1f("u_PointLight.linear",    0.09f);
    // sphereShader.SetUniform1f("u_PointLight.quadratic", 0.032f);

    // set sphere information
    sphereShader.SetUniform1f("u_Radius", sphereRadius);
    sphereShader.SetUniform1f("u_FarDist", cam.GetFar());

    std::string title = "BioSphere Evolution : ";
    float frameUpate = 0.0;
    uint32_t frameCount = 0;

    VertexArray box;
    box.GenVertexBuffer(sizeof(s_cubeVertonly), s_cubeVertonly, 36, GL_STATIC_DRAW);
    box.InsertLayout(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);
    ShaderProgram boxShader("res/shaders/bound_box.vert", "res/shaders/bound_box.frag");

    // MATRICES
    glm::mat4 proj, view, projView;


    
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

        sphereProgram.Use();
        sphereProgram.SetUniformMatrix4fv("u_ProjView", projView);
        sphereProgram.SetUniform3fv("u_CameraPos", cam.GetPosition());
        sphereProgram.SetUniform4fv("u_frustumPlanes", cam.m_Frustum.planes[0], 6);
        sphereProgram.SetUniform1f("u_FarDist", cam.GetFar());
        spheresVAO.Bind();
        glDrawArrays(GL_POINTS, 0, spheresVAO.m_VertBuffer.GetVertAmount());
        
        // rastProgram.Use();
        // rastProgram.SetUniformMatrix4fv("u_ProjView", projView);
        // rastProgram.SetUniformMatrix4fv("u_View", view);
        // rastProgram.SetUniform2iv("u_ScreenSize", glm::ivec2(cam.GetWidth(), cam.GetHeight()));
        // float focalLenght = (float)cam.GetHeight() / (2.0 * glm::tan(cam.GetFovYRad() * 0.5));
        // rastProgram.SetUniform1f("u_FocalLength", focalLenght);
        // rastProgram.SetUniform1f("u_Near", cam.GetNear());
        // rastProgram.SetUniform1f("u_Far", cam.GetFar());
        // rastProgram.SetUniform1ui("u_TotalNumOfSpheres", chunkHolder.spheres.size());
        // rastProgram.SetUniform4fv("u_frustumPlanes", cam.m_Frustum.planes[0], 6);
        
        

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

void print_limits(){
    // limits
    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    printf("Maximum nr of vertex attributes supported: %i \n", nrAttributes);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &nrAttributes);
    printf("Maximum nr of shader storage buffer bindings: %i \n", nrAttributes);
    GLint maxWorkGC[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkGC[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxWorkGC[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWorkGC[2]);
    printf("Max Work Group Count: %i, %i, %i\n", maxWorkGC[0], maxWorkGC[1], maxWorkGC[2]);
    GLint maxWorkGroupSize[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupSize[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkGroupSize[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkGroupSize[2]);
    printf("Max Work Group Size: %i, %i, %i\n", maxWorkGroupSize[0], maxWorkGroupSize[1], maxWorkGroupSize[2]);
    GLint maxWorkGroupInvocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkGroupInvocations);
    printf("Max Work Group Invocations: %i\n", maxWorkGroupInvocations);
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
        glViewport(0, 0, 1, 1);
        m_MainCamera->SetWidthHeight(1, 1);
        // Window is minimized; skip rendering
        return;
    }
    glViewport(0, 0, width, height);
    m_MainCamera->SetWidthHeight(width, height);
    // reset depth texture size
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