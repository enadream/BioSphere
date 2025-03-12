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

static Camera * m_MainCamera;

static float deltaTime;
static double lastX, lastY;

constexpr float sphereRadius = 0.5f;


static glm::mat4 lookToCamera(const glm::vec3 &quad_pos, float radius, float &scale_factor);
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
    glDisable(GL_DEPTH_TEST);
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
    ChunkHolder chunkHolder(100 * CHUNK_SIZE, sphereRadius); // total amount of chunk is x*x
    cam.SetPositionX(0.05 + chunkHolder.GetWidth()*sphereRadius / 2.0f);
    cam.SetPositionY(150.0f);
    cam.SetPositionZ(0.07 + chunkHolder.GetWidth()*sphereRadius / 2.0f);

    printf("Total number of spheres: %llu\n", chunkHolder.spheres.size());
    ComputeConfig::PrintLimits();
    
    ComputeConfig computeConfig;
    computeConfig.CalculateSize(chunkHolder.spheres.size(), 32);
    computeConfig.PrintSizes();

    // load data to the buffer
    VertexArray sphereVAO;
    sphereVAO.GenVertexBuffer(sizeof(Sphere)*chunkHolder.spheres.size(), chunkHolder.spheres.data(), chunkHolder.spheres.size(), GL_STATIC_DRAW);
    sphereVAO.InsertLayout(0, 4, GL_FLOAT, GL_FALSE, sizeof(Sphere), 0);
    // Bind vertex buffer as SSBO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphereVAO.m_VertBuffer.GetID());

    // compute shader
    ShaderProgram drawCompute;
    drawCompute.AttachShader(GL_COMPUTE_SHADER, "res/shaders/draw_ellipse.comp");
    drawCompute.LinkShaders();
    // atomic counters
    Buffer atomicCounters(GL_ATOMIC_COUNTER_BUFFER);
    atomicCounters.GenBuffer(2 * sizeof(uint32_t), nullptr, 2, GL_DYNAMIC_DRAW);
    glBindBufferBase(atomicCounters.GetType(), 1, atomicCounters.GetID());
    // SSBOs
    Buffer spherePoints(GL_SHADER_STORAGE_BUFFER);
    spherePoints.GenBuffer(12 * 1000, nullptr, 1000, GL_DYNAMIC_DRAW);
    glBindBufferBase(spherePoints.GetType(), 2, spherePoints.GetID());
    Buffer sphereQuads(GL_SHADER_STORAGE_BUFFER);
    sphereQuads.GenBuffer(8 * 1000, nullptr, 1000, GL_DYNAMIC_DRAW);
    glBindBufferBase(sphereQuads.GetType(), 3, sphereQuads.GetID());

    /////////// Textures ///////////
    Texture gPosition(GL_TEXTURE_2D, TextureType::UNDEFINED);
    create_texture_2D(gPosition, GL_RGBA32F, cam.GetWidth(), cam.GetHeight(), 0, GL_READ_WRITE);

    Texture gNormal(GL_TEXTURE_2D, TextureType::UNDEFINED);
    create_texture_2D(gNormal, GL_RGBA32F, cam.GetWidth(), cam.GetHeight(), 1, GL_READ_WRITE);

    Texture gIndex(GL_TEXTURE_2D, TextureType::UNDEFINED);
    create_texture_2D(gIndex, GL_R32UI, cam.GetWidth(), cam.GetHeight(), 2, GL_READ_WRITE);

    Texture gDepth(GL_TEXTURE_2D, TextureType::UNDEFINED);
    create_texture_2D(gDepth, GL_R32I, cam.GetWidth(), cam.GetHeight(), 3, GL_READ_WRITE);


    // ShaderProgram pointProgram("res/shaders/ellipse.vert", "res/shaders/ellipse.frag");
    // // directional light
    // pointProgram.Use();
    // pointProgram.SetUniform3fv("u_DirLight.direction", glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f)));
    // pointProgram.SetUniform3fv("u_DirLight.ambient", 0.5f * glm::vec3(1.0f));
    // pointProgram.SetUniform3fv("u_DirLight.diffuse",  0.5f * glm::vec3(1.0f));
    // pointProgram.SetUniform3fv("u_DirLight.specular", 0.5f * glm::vec3(1.0f));


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

    // MATRICES
    glm::mat4 proj, view, projView;

    // sphere size limit
    float pointSizeRange[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, pointSizeRange); // min and max range 

    // rendering z buffer for testing
    ShaderProgram simpleQuadPr("res/shaders/quad.vert", "res/shaders/quad.frag");
    VertexArray simpleQuadVA;
    simpleQuadVA.GenVertexBuffer(sizeof(s_strip_quad), s_strip_quad, 4, GL_STATIC_DRAW);
    simpleQuadVA.InsertLayout(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);
    simpleQuadVA.InsertLayout(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), 3*sizeof(float));

    while(!glfwWindowShouldClose(window)){
        // clear buffer
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        // check texture size
        if (gPosition.GetWidth() != cam.GetWidth()){
            // free old space
            gPosition.Free();
            gPosition.Generate(GL_TEXTURE_2D, TextureType::UNDEFINED);
            gNormal.Free();
            gNormal.Generate(GL_TEXTURE_2D, TextureType::UNDEFINED);
            gIndex.Free();
            gIndex.Generate(GL_TEXTURE_2D, TextureType::UNDEFINED);
            gDepth.Free();
            gDepth.Generate(GL_TEXTURE_2D, TextureType::UNDEFINED);

            // reallocate
            create_texture_2D(gPosition, GL_RGBA32F, cam.GetWidth(), cam.GetHeight(), 0, GL_READ_WRITE);
            create_texture_2D(gNormal, GL_RGBA32F, cam.GetWidth(), cam.GetHeight(), 1, GL_READ_WRITE);
            create_texture_2D(gIndex, GL_R32UI, cam.GetWidth(), cam.GetHeight(), 2, GL_READ_WRITE);
            create_texture_2D(gDepth, GL_R32I, cam.GetWidth(), cam.GetHeight(), 3, GL_READ_WRITE);
        }
        // reset atomic counters
        uint32_t counterValues[2] = {0, 0};
        atomicCounters.Bind();
        glBufferSubData(atomicCounters.GetType(), 0, sizeof(counterValues), counterValues);

        // reset textures
        // the existing image will be check based on depth value, if depth is 2.0 which means the pixel should be all zero
        const int32_t clearVal = glm::floatBitsToInt(2.0f);
        glClearTexImage(gDepth.GetID(), 0, GL_RED_INTEGER, GL_INT, &clearVal);

        const float clearColorRGBA[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        glClearTexImage(gNormal.GetID(), 0, GL_RGBA, GL_FLOAT, &clearColorRGBA);
    
        float focalLenght = (float)cam.GetHeight() / (2.0 * glm::tan(cam.GetFovYRad() * 0.5));
        float oneOverDist = 1.0 / cam.GetFar();
        // setting uniforms
        drawCompute.Use();
        drawCompute.SetUniformMatrix4fv("u_View", view);
        drawCompute.SetUniformMatrix4fv("u_Proj", proj);
        drawCompute.SetUniform3fv("u_CamPos", cam.GetPosition());
        drawCompute.SetUniform3fv("u_CamUp", cam.GetUp());
        drawCompute.SetUniform3fv("u_CamRight", cam.GetRight());
        drawCompute.SetUniform2iv("u_Resolution", glm::ivec2(cam.GetWidth(), cam.GetHeight()));

        drawCompute.SetUniform4fv("u_FrustumPlanes", cam.m_Frustum.planes[0], 6);
        drawCompute.SetUniform1f("u_MaxPointSize", pointSizeRange[1]);
        drawCompute.SetUniform1f("u_OneOverFarDistance", oneOverDist);
        drawCompute.SetUniform1f("u_FocalLength", focalLenght);
        drawCompute.SetUniform1ui("u_TotalNumOfSpheres", chunkHolder.spheres.size());

        glDispatchCompute(computeConfig.DispatchSize.x, computeConfig.DispatchSize.y, computeConfig.DispatchSize.z);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        // test draw
        simpleQuadPr.Use();
        simpleQuadPr.SetUniform1i("u_Tex", 1);
        gNormal.BindTo(1);
        simpleQuadVA.Bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, simpleQuadVA.m_VertBuffer.GetVertAmount());

        // Use uniform buffers for uniforms !!!
        // 3 stage render is needed !!!
        // Step 1 : render sphere's size is smaller than 16*16 using compute shader

        // if render size is bigger than max diameter send for 3rd step render
        // Step 2 : render objects using gl draw points
        // Step 3 : render big spheres using gl draw instanced

        // // focal length in pixel size, so the focalLength in how many pixel size !!!
        // float focalLenght = (float)cam.GetHeight() / (2.0 * glm::tan(cam.GetFovYRad() * 0.5));
        // pointProgram.Use();
        // pointProgram.SetUniformMatrix4fv("u_Proj", proj);
        // pointProgram.SetUniformMatrix4fv("u_View", view);
        // pointProgram.SetUniformMatrix4fv("u_ProjView", projView);
        // pointProgram.SetUniformMatrix4fv("u_InvProj", glm::inverse(proj));
        // pointProgram.SetUniformMatrix4fv("u_InvViewMatrix", glm::inverse(view));

        // pointProgram.SetUniformMatrix3fv("u_CamRotation", glm::transpose(glm::mat3(view)));

        // pointProgram.SetUniform4fv("u_FrustumPlanes", cam.m_Frustum.planes[0], 6);
        // pointProgram.SetUniform1f("u_MaxDiameter", pointSizeRange[1]);
        // pointProgram.SetUniform2iv("u_Resolution", glm::ivec2(cam.GetWidth(), cam.GetHeight()));
        // pointProgram.SetUniform3fv("u_CamPos", cam.GetPosition());
        // pointProgram.SetUniform3fv("u_CamUp", cam.GetUp());
        // pointProgram.SetUniform3fv("u_CamRight", cam.GetRight());
        // pointProgram.SetUniform3fv("u_CamForw", cam.GetFront());
        // pointProgram.SetUniform1f("u_FocalLength", focalLenght);
        // pointProgram.SetUniform1f("u_FarDistance", cam.GetFar());
        // pointProgram.SetUniform1f("u_NearDist", cam.GetNear());
        // pointProgram.SetUniform1i("u_Texture", 1);



        // glm::vec2 halfTan;
        // halfTan.y = glm::tan(cam.GetFovYRad()*0.5);
        // halfTan.x = halfTan.y * cam.GetAspectRatio();
        // pointProgram.SetUniform2fv("u_TanHalfFOV", halfTan);
        //pointProgram.SetUniform2fv("u_HalfOfFOV", glm::vec2((cam.GetFovYRad()*0.5)*cam.GetAspectRatio(), cam.GetFovYRad()*0.5));

        //pointProgram.SetUniform2iv("u_ScreenSize", glm::ivec2(cam.GetWidth(), cam.GetHeight()));

        //pointProgram.SetUniformMatrix4fv("invViewMatrix", glm::inverse(view));

        // pointProgram.SetUniform3fv("u_CameraFront", cam.GetFront());
        // pointProgram.SetUniform3fv("u_CamUp", cam.GetUp());
        // pointProgram.SetUniform3fv("u_CamRight", cam.GetRight());
        //sphereTex.BindTo(1);
        // sphereVAO.Bind();
        // glDrawArrays(GL_POINTS, 0, sphereVAO.m_VertBuffer.GetVertAmount());

        // rastProgram.Use();
        // rastProgram.SetUniformMatrix4fv("u_View", projView);
        // rastProgram.SetUniformMatrix4fv("u_View", view);
        // rastProgram.SetUniform2iv("u_ScreenSize", glm::ivec2(cam.GetWidth(), cam.GetHeight()));
        
        // rastProgram.SetUniform1f("u_FocalLength", focalLenght);
        // rastProgram.SetUniform1f("u_Near", cam.GetNear());
        // rastProgram.SetUniform1f("u_Far", cam.GetFar());
        // rastProgram.SetUniform1ui("u_TotalNumOfSpheres", chunkHolder.spheres.size());
        // rastProgram.SetUniform4fv("u_frustumPlanes", cam.m_Frustum.planes[0], 6);
        

        

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