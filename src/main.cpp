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

#include "camera.hpp"
#include "temp_data.hpp"
#include "shader.hpp"
#include "model.hpp"
#include "vertex_array.hpp"
#include "math/PerlinNoise.hpp"
#include "math/simplex.h"
#include "noise.hpp"

static glm::mat4 projection;
static Camera * m_MainCamera;

static float deltaTime;
static int m_Width = 800, m_Height = 600;
static double lastX, lastY;

constexpr float near_distance = 0.5f;
constexpr float far_distance = 10000.0f;


#include "debug_func.hpp"

static glm::mat4 lookToCamera(const glm::vec3 &quad_pos, float radius, float &scale_factor);
void frame_buffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
float get_block_height(glm::vec3 &block_pos, float radius);
float get_block_height_perlin(glm::vec3 &block_pos, float radius, int floor);
float generateHeight(const glm::vec3& position);
float terrain_height(const glm::vec3& position);

int main(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
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

    glEnable(GL_MULTISAMPLE);
    // set viewport coordinats first 2 value are lower left corner of the window
    glViewport(0, 0, 800, 600);
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glEnable(GL_CULL_FACE);
    // blending
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetFramebufferSizeCallback(window, frame_buffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    Camera cam(glm::vec3(0.30f, 40.05f, 0.30f));
    m_MainCamera = &cam;

    // View Matrix
    glm::mat4 view = cam.GetViewMatrix();
    // Projection Matrix
    projection = glm::perspective(glm::radians(m_MainCamera->m_Fov), (float)m_Width/m_Height, near_distance, far_distance);


    // create sphere objects
    std::vector<glm::vec3> positions;
    
    constexpr uint32_t xAmount = 10;
    constexpr uint32_t yAmount = 10;
    constexpr uint32_t zAmount = 20;
    positions.reserve(xAmount * yAmount * zAmount);

    constexpr float sphereRadius = 0.5f;
    constexpr float diameter = 2*sphereRadius;
    
    // offset values
    constexpr float xCenterOff = diameter*xAmount/2.0f;
    constexpr float zCenterOff = sphereRadius*zAmount/2.0f;

    glm::vec3 spawnPos;
    // overlapping exist! top level is same as height level
    for (int64_t i = 0; i < yAmount; i++){
        spawnPos.y = -i * sphereRadius;
        for (uint32_t j = 0; j < zAmount; j++){
            spawnPos.z = j * sphereRadius - zCenterOff;
            float xOffset = (j + i) % 2 == 0 ? 0.0f : sphereRadius; // (j + i)
            for (uint32_t k = 0; k < xAmount; k++){
                spawnPos.x = k * diameter + xOffset - xCenterOff;
                // spawnPos.y = get_block_height_perlin(spawnPos, sphereRadius, i);
                //spawnPos.y = generateHeight(spawnPos);
                //spawnPos.y = getTerrainHeight(spawnPos.x, spawnPos.z, (i%2)*sphereRadius);
                positions.emplace_back(spawnPos);
            }
        }
    }


    // constexpr uint32_t terrainX = xAmount+2;
    // constexpr uint32_t terrainZ = zAmount+2;
    // std::vector<std::vector<float>> terrainHeight;
    // terrainHeight.reserve(terrainZ);

    // // find terrain discrete hights
    // for (uint32_t i = 0; i < terrainZ; i++){
    //     terrainHeight.push_back();
    //     terrainHeight[i].reserve(terrainX);
    //     float zValue = i*sphereRadius;

    //     for (uint32_t j = 0; j < terrainX; j++){
    //         float xValue = j*sphereRadius;
    //         float yOffset = ((i+j) % 2) * sphereRadius;
    //         float yValue = getTerrainHeight(xValue, zValue) + yOffset;
    //         uint64_t discrete = yValue / sphereRadius;
    //         yValue = discrete * sphereRadius - yOffset;
    //         terrainHeight[i].push_back(yValue);
    //     }
    // }

    // // create lower layer by using upper layer
    // for (uint32_t i = 2; i < yAmount; i++){
    //     for (uint32_t j = 0; j < zAmount; j++){
    //         for (uint32_t k = 0; k < xAmount; k++){
    //             uint32_t targetId = (i%2)*zAmount*xAmount + j*xAmount + k;
    //             spawnPos.x = positions[targetId].x;
    //             spawnPos.z = positions[targetId].z;
    //             spawnPos.y = positions[targetId].y - sphereRadius;
    //             positions.emplace_back(spawnPos);
    //         }
    //     }
    // }

    // load data to the vertex buffer
    VertexArray sphere;
    sphere.GenVertexBuffer(positions.size() * sizeof(glm::vec3), (void*)positions.data(), positions.size(), GL_STATIC_DRAW);
    sphere.InsertLayout(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    //sphere.GenIndexBuffer(sizeof(s_quad_inds), (void*)s_quad_inds, IndexType::UINT8, GL_STATIC_DRAW);

    // Sphere TEXTURES
    vector<string> spFaces = 
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
    vector<string> faces = 
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

    // shaders
    Shader skyboxShader("res/shaders/skybox.vert", "res/shaders/skybox.frag");
    Shader sphereShader("res/shaders/sphere.vert", "res/shaders/sphere.frag", "res/shaders/sphere.geom");
    sphereShader.Use();
    sphereShader.SetUniformMatrix4fv("u_Projection", projection);
    
    // light information
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f));
    glm::vec3 lightColor(1.0f);

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
    sphereShader.SetUniform1f("u_FarDist", far_distance);

    std::string title = "BioSphere Evolution : ";
    float frameUpate = 0.0;
    uint32_t frameCount = 0;

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
        view = cam.GetViewMatrix();

        // set uniform data of sphere shader
        sphereShader.Use();
        sphereShader.SetUniformMatrix4fv("u_Projection", projection);
        sphereShader.SetUniformMatrix4fv("u_View", view);

        // set camera information
        sphereShader.SetUniform3fv("u_CameraPos", cam.m_Position);
        
        //sphereShader.SetUniform3fv("u_DirLight.ambient", 0.3f * glm::vec3(1.0f));
        //sphereShader.SetUniform3fv("u_DirLight.diffuse",  0.5f * glm::vec3(1.0f));
        //sphereShader.SetUniform3fv("u_DirLight.specular", 0.5f * glm::vec3(1.0f));

        sphereShader.SetUniform3fv("u_Color", glm::vec3(0.5f, 0.8f, 1.0f));
        sphereShader.SetUniform1i("u_Texture", 1);
        sphereTex.BindTo(1);

        // draw spheres
        sphere.Bind();
        glDrawArrays(GL_POINTS, 0, sphere.m_VertBuffer.GetVertAmount());

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

float get_block_height_perlin(glm::vec3 &block_pos, float radius, int floor){
    const noise::PerlinNoise::seed_type seed = 0xe5a6d75d;
    const noise::PerlinNoise perlin{seed};
    
    constexpr float frequency = 0.01f;
    constexpr float amplitude = 50.0f;

    float height = perlin.octave2D(block_pos.x * frequency, block_pos.z * frequency, 4) * (amplitude/2.0f);
    //int32_t count = height / radius;
    //count -= floor;

    return height;
}

float get_block_height(glm::vec3 &block_pos, float radius) {
    constexpr float frequency = 0.1f;
    constexpr float amplitude = 5.0f;

    float xOffset = glm::sin(block_pos.x * frequency) * amplitude;
    float zOffset = glm::sin(block_pos.z * frequency) * amplitude;
    float height = xOffset + zOffset;
    int multiple = height / radius;
    return height;
}

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

// Pseudo-random number generation
float myRandomMagic(const vec3& p) {
    float dt = dot(p, vec3(12.9898f, 78.233f, 45.164f));
    return fract(sin(dt) * 43758.5453f);
}

// 3D Value Noise with derivatives
vec4 noised(const vec3& x) {
    vec3 p = floor(x);
    vec3 w = fract(x);

    // Quintic interpolation polynomial
    vec3 u = w * w * w * (w * (w * 6.0f - 15.0f) + 10.0f);
    vec3 du = 30.0f * w * w * (w - 1.0f) * (w - 1.0f);

    // Get random values at lattice points
    float a = myRandomMagic(p + vec3(0,0,0));
    float b = myRandomMagic(p + vec3(1,0,0));
    float c = myRandomMagic(p + vec3(0,1,0));
    float d = myRandomMagic(p + vec3(1,1,0));
    float e = myRandomMagic(p + vec3(0,0,1));
    float f = myRandomMagic(p + vec3(1,0,1));
    float g = myRandomMagic(p + vec3(0,1,1));
    float h = myRandomMagic(p + vec3(1,1,1));

    // Interpolation coefficients
    float k0 = a;
    float k1 = b - a;
    float k2 = c - a;
    float k3 = e - a;
    float k4 = a - b - c + d;
    float k5 = a - c - e + g;
    float k6 = a - b - e + f;
    float k7 = -a + b + c - d + e - f - g + h;

    // Noise value [-1, 1]
    float value = k0 + k1*u.x + k2*u.y + k3*u.z 
                + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z;
    value = -1.0f + 2.0f * value;

    // Derivatives
    vec3 derivatives;
    derivatives.x = (k1 + k4*u.y + k6*u.z + k7*u.y*u.z) * du.x;
    derivatives.y = (k2 + k5*u.z + k4*u.x + k7*u.z*u.x) * du.y;
    derivatives.z = (k3 + k6*u.x + k5*u.y + k7*u.x*u.y) * du.z;

    return vec4(value, derivatives);
}

// Terrain height function using x/z coordinates
float terrain_height(const vec3& position) {
    constexpr float frequency = 0.2f;

    vec2 p(position.x * frequency, position.z * frequency); // Scale input position appropriately
    float height = 0.0f;
    float amplitude = 4.0f;
    vec2 derivatives = vec2(0.0f);
    mat2 rotation = mat2(0.8f, -0.6f, 0.6f, 0.8f); // 2D rotation matrix
    const int octaves = 4;

    for(int i = 0; i < octaves; i++) {
        vec4 noise = noised(vec3(p, 0.0f)); // Sample at current 2D position
        
        // Accumulate derivatives and modify height calculation
        derivatives += vec2(noise.y, noise.w); // dx/dz derivatives
        height += amplitude * noise.x / (1.0f + dot(derivatives, derivatives));
        
        // Update parameters for next octave
        amplitude *= 0.5f;
        p = rotation * p * 2.0f; // Rotate and scale position
    }

    return height;
}

// float generateHeight(const glm::vec3& position) {
//     constexpr float frequency = 0.01f;
//     constexpr float amplitude = 50.0f;
//     // Parameters for fractal Brownian motion
//     const int octaves = 4;
//     const float lacunarity = 2.0f;
//     const float gain = 0.5f;

//     // Generate fractal Brownian motion noise based on the input position
//     float height = Simplex::fBm(glm::vec2(position.x, position.z)*frequency, octaves, lacunarity, gain) * amplitude;

//     return height;
// }

float generateHeight(const glm::vec3& position) {
    constexpr float baseFrequency = 0.005f;
    constexpr float baseAmplitude = 50.0f;

    constexpr float detailFrequency = 0.05f;
    constexpr float detailAmplitude = 0.0f;

    // Generate base terrain (large-scale features)
    float baseHeight = Simplex::fBm(glm::vec2(position.x, position.z) * baseFrequency, 4, 2.0f, 0.5f) * baseAmplitude;

    // Generate details (small-scale noise)
    float detailHeight = Simplex::fBm(glm::vec2(position.x, position.z) * detailFrequency, 3, 2.0f, 0.5f) * detailAmplitude;

    // Combine base terrain and details
    float height = baseHeight + detailHeight;

    return height;
}







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
    projection = glm::perspective(glm::radians(m_MainCamera->m_Fov), (float)m_Width/m_Height, near_distance, far_distance);
}

void frame_buffer_size_callback(GLFWwindow* window, int width, int height){
    if (width == 0 || height == 0) {
        // Window is minimized; skip rendering
        return;
    }
    glViewport(0, 0, width, height);
    m_Width = width;
    m_Height = height;
    projection = glm::perspective(glm::radians(m_MainCamera->m_Fov), (float)m_Width/m_Height, near_distance, far_distance);
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

static glm::mat4 lookToCamera(const glm::vec3 &quad_pos, float radius, float &scale_factor){

    // Calculate direction to the camera
    glm::vec3 distVector = m_MainCamera->m_Position - quad_pos;
    glm::vec3 direction = glm::normalize(distVector);

    // Compute right and up vectors for the quad
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0, 1.0, 0.0), direction)); // m_MainCamera->m_WorldUp
    glm::vec3 up = glm::cross(direction, right);

    // Create the rotation matrix
    glm::mat4 rotation = glm::mat4(1.0f);
    rotation[0] = glm::vec4(right, 0.0f);
    rotation[1] = glm::vec4(up, 0.0f);
    rotation[2] = glm::vec4(direction, 0.0f); // Negative to face the camera

    // Create the translation matrix
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), quad_pos);

    // scale
    float distance = glm::length(distVector);
    glm::mat4 scale;
    

    if (distance > 1.1*radius){
        scale_factor = distance / glm::sqrt((distance-radius)*(distance+radius));
        scale = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor, scale_factor, scale_factor));
    } else {
        scale_factor = 1.0;
        scale = glm::mat4(1.0f);
    }

    
    // Combine translation and rotation
    glm::mat4 model = translation * rotation * scale;


    // Debug output
    // std::cout << "Direction: " << vec3ToString(direction) << std::endl;
    // std::cout << "Model Matrix: " << mat4ToString(model) << std::endl;
    

    return model;
}