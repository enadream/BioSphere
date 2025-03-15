#include <stdio.h>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <vector>
#include <cstring>    // For memcpy
#include <iostream>
#include <sstream>

// opengl math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>

#include "debug_func.hpp"
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
#include "compute_handler.hpp"
#include "memory_manager.hpp"

static Camera * m_MainCamera;

static float deltaTime;
static double lastX, lastY;

constexpr float sphereRadius = 0.5f;

void frame_buffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void create_texture_2D(Texture &texture, uint32_t internal_format, int32_t width, int32_t height, uint32_t bind_index, uint32_t access);

std::string vec3ToString(const glm::vec3& vec);

// // Enable dedicated graphics for NVIDIA:
// extern "C" 
// {
//   __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
// }
// // Enable dedicated graphics for AMD Radeon:
// extern "C"
// {
//   __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
// }

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

    glEnable(GL_PROGRAM_POINT_SIZE);

    glfwSetFramebufferSizeCallback(window, frame_buffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);


    //////////////// CHUNKS HOLDER ////////////////////////////////////////////////////////////////////////////////////////////////
    ChunkHolder chunkHolder(100, sphereRadius); // total amount of chunk is x*x
    cam.SetPositionX(0.05 + chunkHolder.GetWidthChunkAmount()*CHUNK_SIZE*0.5*sphereRadius);
    cam.SetPositionY(150.0f);
    cam.SetPositionZ(0.07 + chunkHolder.GetWidthChunkAmount()*CHUNK_SIZE*0.5*sphereRadius);

    printf("Total number of spheres: %u\n", chunkHolder.GetTotalNumOfSpheres());
    ComputeConfig::PrintLimits();
    
    ComputeConfig computeConfig;
    computeConfig.CalculateSize(chunkHolder.GetTotalNumOfSpheres(), 32);
    computeConfig.PrintSizes();

    // buffer memory manager
    MemoryManager memoryManager(1.2 * chunkHolder.GetTotalNumOfSpheres() * sizeof(Sphere));

    // load data to the buffer
    VertexArray sphereVAO;
    sphereVAO.m_VertBuffer.GenVertexBuffer(memoryManager.GetTotalSize(), nullptr, chunkHolder.GetTotalNumOfSpheres(), GL_STATIC_DRAW);
    sphereVAO.InsertLayout(0, 4, GL_FLOAT, GL_FALSE, sizeof(Sphere), 0);
    
    sphereVAO.Bind();
    for (uint32_t i = 0; i < chunkHolder.chunks.size(); i++){
        uint32_t offset = 0;
        uint32_t byte_size = chunkHolder.chunks[i].spheres.size()*sizeof(Sphere);
        memoryManager.Allocate(byte_size, offset);
        // to find the vertex offset, we need to divide the byte offset by the vertex size
        chunkHolder.chunks[i].bufferVertexOffset = offset / sizeof(Sphere);
        glBufferSubData(GL_ARRAY_BUFFER, offset, byte_size, chunkHolder.chunks[i].spheres.data());
    }


    // uint32_t sphereOffsetBytes = 0;
    // for (uint32_t i = 0; i < chunkHolder.chunks.size(); i++){
    //     glBufferSubData(GL_ARRAY_BUFFER, sphereOffsetBytes, chunkHolder.chunks[i].spheres.size()*sizeof(Sphere),
    //         chunkHolder.chunks[i].spheres.data());
    //     sphereOffsetBytes += chunkHolder.chunks[i].spheres.size() * sizeof(Sphere);
    // }
    
    // sphereVAO.m_VertBuffer.GenVertexBufferStorage(memoryManager.GetTotalSize(), nullptr, chunkHolder.GetTotalNumOfSpheres(),
    //     GL_DYNAMIC_STORAGE_BIT);
    // //sphereVAO.m_VertBuffer.GenPersistentBuffer(0, memoryManager.GetTotalSize(), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    // sphereVAO.InsertLayout(0, 4, GL_FLOAT, GL_FALSE, sizeof(Sphere), 0);

    // sphereVAO.Bind();
    // uint32_t sphereOffsetBytes = 0;
    // for (uint32_t i = 0; i < chunkHolder.chunks.size(); i++){
    //     glBufferSubData(GL_ARRAY_BUFFER, sphereOffsetBytes, chunkHolder.chunks[i].spheres.size()*sizeof(Sphere),
    //         chunkHolder.chunks[i].spheres.data());
    //     sphereOffsetBytes += chunkHolder.chunks[i].spheres.size() * sizeof(Sphere);
    // }

    // // load each chunk data
    // sphereVAO.m_VertBuffer.Bind();
    // for (uint32_t i = 0; i < chunkHolder.chunks.size(); i++){
    //     uint32_t offset = 0;
    //     uint32_t byte_size = chunkHolder.chunks[i].spheres.size()*sizeof(Sphere);
    //     memoryManager.Allocate(byte_size, offset);
    //     // to find the vertex offset, we need to divide the byte offset by the vertex size
    //     chunkHolder.chunks[i].bufferVertexOffset = offset / sizeof(Sphere);
    //     // Copy chunk data into the persistent mapped buffer at the current offset.
    //     memcpy(static_cast<char*>(sphereVAO.m_VertBuffer.GetMap())+offset, chunkHolder.chunks[i].spheres.data(), byte_size);
    // }
    

    // sphereVAO.Unbind();
    // sphereVAO.m_VertBuffer.Unbind();

    // // multidraw command buffer
    Buffer drawCommandBuffer(GL_DRAW_INDIRECT_BUFFER);
    drawCommandBuffer.GenBuffer(chunkHolder.GetTotalChunkAmount()*sizeof(DrawArraysIndirectCommand), nullptr, 0, GL_DYNAMIC_DRAW);
    // Buffer drawCommandBuffer(GL_DRAW_INDIRECT_BUFFER);
    // drawCommandBuffer.GenBufferStorage(chunkHolder.GetTotalChunkAmount()*sizeof(DrawArraysIndirectCommand), nullptr, 
    //     chunkHolder.GetTotalChunkAmount(), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    // drawCommandBuffer.GenPersistentBuffer(0, drawCommandBuffer.GetSize(), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    ShaderProgram atomProgram("res/shaders/atom.vert", "res/shaders/atom.frag");
    // directional light
    atomProgram.Use();
    atomProgram.SetUniform3fv("u_DirLight.direction", glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f)));
    atomProgram.SetUniform3fv("u_DirLight.ambient", 0.5f * glm::vec3(1.0f));
    atomProgram.SetUniform3fv("u_DirLight.diffuse",  0.5f * glm::vec3(1.0f));
    atomProgram.SetUniform3fv("u_DirLight.specular", 0.5f * glm::vec3(1.0f));


    std::string title = "BioSphere Evolution : ";
    float frameUpate = 0.0;
    uint32_t frameCount = 0;

    // Sphere TEXTURES
    std::vector<string> spFaces =
    {
        "res/textures/dirt_block/dirt.png",
        "res/textures/dirt_block/dirt.png",
        "res/textures/dirt_block/dirt.png",
        "res/textures/dirt_block/dirt.png",
        "res/textures/dirt_block/dirt.png",
        "res/textures/dirt_block/dirt.png"
    };

    CubeTexture sphereTex;
    sphereTex.LoadCubeMap(spFaces);

    VertexArray box;
    box.GenVertexBuffer(sizeof(s_cubeVertonly), s_cubeVertonly, 36, GL_STATIC_DRAW);
    box.InsertLayout(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);
    ShaderProgram boxShader("res/shaders/bound_box.vert", "res/shaders/bound_box.frag");

    // MATRICES
    glm::mat4 proj, view, projView;

    // sphere size limit
    float pointSizeRange[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, pointSizeRange); // min and max range 

    std::vector<DrawArraysIndirectCommand> drawCommands;
    drawCommands.reserve(chunkHolder.GetTotalChunkAmount());

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

       
        float focalLenght = (float)cam.GetHeight() / (2.0 * glm::tan(cam.GetFovYRad() * 0.5));
        float oneOverDist = 1.0 / cam.GetFar();
        // setting uniforms
        atomProgram.Use();
        atomProgram.SetUniformMatrix4fv("u_View", view);
        atomProgram.SetUniformMatrix4fv("u_Proj", proj);
        atomProgram.SetUniform3fv("u_CamPos", cam.GetPosition());
        atomProgram.SetUniform3fv("u_CamUp", cam.GetUp());
        atomProgram.SetUniform3fv("u_CamRight", cam.GetRight());
        atomProgram.SetUniform2iv("u_Resolution", glm::ivec2(cam.GetWidth(), cam.GetHeight()));

        atomProgram.SetUniform4fv("u_FrustumPlanes", cam.m_Frustum.planes[0], 6);
        atomProgram.SetUniform1f("u_MaxPointSize", pointSizeRange[1]);
        atomProgram.SetUniform1f("u_OneOverFarDistance", oneOverDist);
        atomProgram.SetUniform1f("u_FocalLength", focalLenght);

        
        // accomplish frustum culling
        drawCommands.clear();
        for (uint32_t i = 0; i < chunkHolder.chunks.size(); i++){
            if (chunkHolder.chunks[i].boundBox.IsOnFrustum(cam.m_Frustum)){
                drawCommands.emplace_back(chunkHolder.chunks[i].spheres.size(), 1, chunkHolder.chunks[i].bufferVertexOffset, 0);

                // DrawArraysIndirectCommand commandBuff;
                // commandBuff.count = chunkHolder.chunks[i].spheres.size();
                // commandBuff.first = chunkHolder.chunks[i].bufferVertexOffset;
                // //  we don't use instanced rendering
                // commandBuff.instanceCount = 1;
                // commandBuff.baseInstance = 0;
                // // copy value to the buffer
                // uint32_t byteOffset = drawAmount*sizeof(DrawArraysIndirectCommand);
                // memcpy(static_cast<char*>(drawCommandBuffer.GetMap())+byteOffset, &commandBuff, sizeof(DrawArraysIndirectCommand));
                // drawAmount++;
            }
        }

        drawCommandBuffer.Bind();
        // copy to indirect buffer
        glBufferSubData(drawCommandBuffer.GetType(), 0, drawCommands.size()*sizeof(DrawArraysIndirectCommand), drawCommands.data());
        sphereVAO.Bind();
        glMultiDrawArraysIndirect(GL_POINTS, (void*)0, drawCommands.size(), 0);
        //glDrawArrays(GL_POINTS, 0, chunkHolder.GetTotalNumOfSpheres());

        // boxShader.Use();
        // box.Bind();
        // boxShader.SetUniformMatrix4fv("u_ProjView", projView);

        // for (uint32_t i = 0; i < chunkHolder.chunks.size(); i++){
        //     glm::mat4 modelBoundBox = glm::mat4(1.0f);
        //     modelBoundBox = glm::translate(modelBoundBox, chunkHolder.chunks[i].boundBox.GetCenter());
        //     modelBoundBox = glm::scale(modelBoundBox, chunkHolder.chunks[i].boundBox.GetSize());
        //     boxShader.SetUniformMatrix4fv("u_Model", modelBoundBox);
        //     glDrawArrays(GL_TRIANGLES, 0, box.m_VertBuffer.GetVertAmount());
        // }
        

        // // After updating your persistently mapped indirect buffer:
        // GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

        // // Wait for the GPU to catch up.
        // // Here we block until the fence is signaled (with a 1-second timeout).
        // GLenum waitResult = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000);
        // if (waitResult == GL_ALREADY_SIGNALED || waitResult == GL_CONDITION_SATISFIED) {
        //     // Now it's safe to issue the draw call.
        //     sphereVAO.Bind();
        //     glMultiDrawArraysIndirect(GL_POINTS, (void*)0, drawAmount, 0);
        // } else {
        //     // Handle the error or timeout (e.g., log, skip frame, etc.)
        //     printf("[ERROR]: GPU sync timed out or failed.\n");
        // }
        // // Delete the fence.
        // glDeleteSync(fence);
        
        


        // Use uniform buffers for uniforms !!!
        // 3 stage render is needed !!!
        // Step 1 : render sphere's size is smaller than 16*16 using compute shader

        // if render size is bigger than max diameter send for 3rd step render
        // Step 2 : render objects using gl draw points
        // Step 3 : render big spheres using gl draw instanced


        

        glfwSwapBuffers(window);
        // check and call events and swap the buffers
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void create_texture_2D(Texture &texture, uint32_t internal_format, int32_t width, int32_t height, uint32_t bind_index, uint32_t access){
    texture.GLTexStorage2D(1, internal_format, width, height);
    texture.SetTexParametrI(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    texture.SetTexParametrI(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    texture.SetTexParametrI(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    texture.SetTexParametrI(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindImageTexture(bind_index, texture.GetID(), 0, GL_FALSE, 0, access, internal_format);
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

    float cameraSpeed = 2.0f; // adjust accordingly

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