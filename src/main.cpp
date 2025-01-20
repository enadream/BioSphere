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

#include "camera.hpp"
#include "temp_data.hpp"
#include "shader.hpp"
#include "model.hpp"
#include "vertex_array.hpp"

static glm::mat4 projection;
static Camera * m_MainCamera;

static float deltaTime;
static int m_Width = 800, m_Height = 600;
static double lastX, lastY;

constexpr float near_distance = 0.5f;
constexpr float far_distance = 500.0f;


#include "debug_func.hpp"

static glm::mat4 lookToCamera(const glm::vec3 &quad_pos, float radius, float &scale_factor);


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
        cameraSpeed += 10.0f;
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    m_MainCamera->ProcessMouseScroll(yoffset);
    projection = glm::perspective(glm::radians(m_MainCamera->m_Fov), (float)m_Width/m_Height, near_distance, far_distance);
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

    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    printf("Maximum nr of vertex attributes supported: %i \n", nrAttributes);

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

    Camera cam(glm::vec3(0.0f, 3.0f, 0.0f));
    m_MainCamera = &cam;

    // View Matrix
    glm::mat4 view = cam.GetViewMatrix();
    // Projection Matrix
    projection = glm::perspective(glm::radians(m_MainCamera->m_Fov), (float)m_Width/m_Height, near_distance, far_distance);
    //glm::ortho(0.0f, (float)m_Width, 0.0f, (float)m_Height, -1.0f, 1.0f);
    //glm::perspective(glm::radians(cam.m_Fov), (float)m_Width/m_Height, 1.0f, 100.0f);

    

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

    // light information
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f));
    glm::vec3 lightColor(1.0f);

    glm::vec3 pointLightPositions[] = {
	    glm::vec3( 0.7f,  0.2f,  2.0f),
	    glm::vec3( 2.3f, -3.3f, -4.0f),
	    glm::vec3(-4.0f,  2.0f, -12.0f),
	    glm::vec3( 0.0f,  0.0f, -3.0f)
    };

    // // create vertex mesh for cube
    // Mesh cubeMesh;
    // for (uint32_t i = 0; i < sizeof(s_vertices2)/sizeof(float); i+=8){
    //     cubeMesh.m_Vertices.emplace_back(&s_vertices2[i]);
    //     cubeMesh.m_Indices.emplace_back(i);
    // }
    // cubeMesh.LoadMesh();

    
    // // load textures
    // cubeMesh.m_Texures.emplace_back("res/textures/container2.png", TextureType::DIFFUSE);
    // cubeMesh.m_Texures.emplace_back("res/textures/container2_specular.png", TextureType::SPECULAR);
    // cubeMesh.m_Texures.emplace_back("res/textures/matrix_emission.png", TextureType::EMISSION);

    // cubeMesh.m_Texures.emplace_back("res/textures/wall.jpg", TextureType::DIFFUSE);
    // cubeMesh.m_Texures.emplace_back("res/textures/container2_specular.png", TextureType::SPECULAR);
    // cubeMesh.m_Texures.emplace_back("res/textures/matrix_emission.png", TextureType::EMISSION);

    // load light's shader
    Shader lightShader("res/shaders/cube.vert", "res/shaders/light.frag");
    lightShader.Use();
    lightShader.SetUniform3fv("u_LightColor", lightColor);

    // shaders
    Shader shader("res/shaders/cube.vert", "res/shaders/cube.frag");
    shader.Use();

    // shader.SetUniform1i("u_Material.DiffuseTexture", 0);
    // shader.SetUniform1i("u_Material.SpecularTexture", 1);
    // shader.SetUniform1i("u_Material.EmissionTexture", 2);
    shader.SetUniform1f("u_Material.Shininess", 64.0f);

    // directional light
    shader.SetUniform3fv("u_DirLight.direction", lightDir);
    shader.SetUniform3fv("u_DirLight.ambient", 0.05f * lightColor);
    shader.SetUniform3fv("u_DirLight.diffuse",  0.4f * lightColor);
    shader.SetUniform3fv("u_DirLight.specular", 0.5f * lightColor);
    
    // point lights
    shader.SetUniform3fv("u_PointLights[0].position",  pointLightPositions[0]);
    shader.SetUniform3fv("u_PointLights[0].ambient", 0.3f * lightColor);
    shader.SetUniform3fv("u_PointLights[0].diffuse",  0.6f * lightColor);
    shader.SetUniform3fv("u_PointLights[0].specular", lightColor);
    shader.SetUniform1f("u_PointLights[0].constant",  1.0f);
    shader.SetUniform1f("u_PointLights[0].linear",    0.09f);
    shader.SetUniform1f("u_PointLights[0].quadratic", 0.032f);

    shader.SetUniform3fv("u_PointLights[1].position",  pointLightPositions[1]);
    shader.SetUniform3fv("u_PointLights[1].ambient", 0.05f * lightColor);
    shader.SetUniform3fv("u_PointLights[1].diffuse",  0.8f * lightColor);
    shader.SetUniform3fv("u_PointLights[1].specular", lightColor);
    shader.SetUniform1f("u_PointLights[1].constant",  1.0f);
    shader.SetUniform1f("u_PointLights[1].linear",    0.09f);
    shader.SetUniform1f("u_PointLights[1].quadratic", 0.032f);

    shader.SetUniform3fv("u_PointLights[2].position",  pointLightPositions[2]);
    shader.SetUniform3fv("u_PointLights[2].ambient", 0.05f * lightColor);
    shader.SetUniform3fv("u_PointLights[2].diffuse",  0.8f * lightColor);
    shader.SetUniform3fv("u_PointLights[2].specular", lightColor);
    shader.SetUniform1f("u_PointLights[2].constant",  1.0f);
    shader.SetUniform1f("u_PointLights[2].linear",    0.09f);
    shader.SetUniform1f("u_PointLights[2].quadratic", 0.032f);

    shader.SetUniform3fv("u_PointLights[3].position",  pointLightPositions[3]);
    shader.SetUniform3fv("u_PointLights[3].ambient", 0.05f * lightColor);
    shader.SetUniform3fv("u_PointLights[3].diffuse",  0.8f * lightColor);
    shader.SetUniform3fv("u_PointLights[3].specular", lightColor);
    shader.SetUniform1f("u_PointLights[3].constant",  1.0f);
    shader.SetUniform1f("u_PointLights[3].linear",    0.09f);
    shader.SetUniform1f("u_PointLights[3].quadratic", 0.032f);
    
    // spot light
    shader.SetUniform3fv("u_SpotLight.position", cam.m_Position);
    shader.SetUniform3fv("u_SpotLight.direction", cam.m_Front);
    shader.SetUniform3fv("u_SpotLight.ambient", 0.0f, 0.0f, 0.0f);
    shader.SetUniform3fv("u_SpotLight.diffuse", 1.0f, 0.0f, 0.0f);
    shader.SetUniform3fv("u_SpotLight.specular", 1.0f, 0.0f, 0.0f);
    shader.SetUniform1f("u_SpotLight.constant", 1.0f);
    shader.SetUniform1f("u_SpotLight.linear", 0.09f);
    shader.SetUniform1f("u_SpotLight.quadratic", 0.032f);
    shader.SetUniform1f("u_SpotLight.cutOff", glm::cos(glm::radians(12.5f)));
    shader.SetUniform1f("u_SpotLight.outerCutOff", glm::cos(glm::radians(15.0f)));


    Model ourModel;
    // ourModel.LoadModel("res/models/nanosuit/nanosuit.obj");
    // ourModel.LoadAllMeshes();
    // ourModel.LoadTextures(shader);

    Shader quadShader("res/shaders/quad.vert", "res/shaders/sphere.frag");
    quadShader.Use();
    quadShader.SetUniformMatrix4fv("u_Projection", projection);
    
    // directional light
    quadShader.SetUniform3fv("u_DirLight.direction", glm::normalize(lightDir));
    quadShader.SetUniform3fv("u_DirLight.ambient", 0.5f * glm::vec3(1.0f));
    quadShader.SetUniform3fv("u_DirLight.diffuse",  0.5f * glm::vec3(1.0f));
    quadShader.SetUniform3fv("u_DirLight.specular", 0.5f * glm::vec3(1.0f));

    quadShader.SetUniform3fv("u_PointLight.position",  glm::vec3(0.0f, 5.0f, 0.0f));
    quadShader.SetUniform3fv("u_PointLight.ambient", 0.05f * lightColor);
    quadShader.SetUniform3fv("u_PointLight.diffuse",  0.8f * lightColor);
    quadShader.SetUniform3fv("u_PointLight.specular", lightColor);
    quadShader.SetUniform1f("u_PointLight.constant",  1.0f);
    quadShader.SetUniform1f("u_PointLight.linear",    0.09f);
    quadShader.SetUniform1f("u_PointLight.quadratic", 0.032f);

   


    // create sphere objects
    std::vector<glm::vec3> positions;
    

    constexpr uint32_t xAmount = 10;
    constexpr uint32_t yAmount = 10;
    constexpr uint32_t zAmount = 20;
    positions.reserve(xAmount * yAmount * zAmount);

    constexpr float innerRadius = 0.5f;
    constexpr float outerRadius = 0.5f;
    glm::vec3 spawnPos;

    constexpr float circleHeight = innerRadius * glm::sqrt(3); // sqrt(3) = 1.7320508076
    constexpr float diameter = 2*innerRadius;

    constexpr float xCenterOff = diameter*xAmount/2.0f;
    constexpr float zCenterOff = circleHeight*zAmount/2.0f;


    // overlapping exist! top level is same as height level
    for (int i = 0; i < yAmount; i++){
        spawnPos.y = - i * innerRadius;
        for (int j = 0; j < zAmount; j++){
            spawnPos.z = j * innerRadius - zCenterOff;
            float xOffset = (j + i) % 2 == 0 ? 0.0f : innerRadius;
            for (int k = 0; k < xAmount; k++){
                spawnPos.x = k * diameter + xOffset - xCenterOff;
                positions.emplace_back(spawnPos);
            }
        }
    }
    
    // // overlapping exist! top level is hexagonal
    // for (int i = 0; i < yAmount; i++){
    //     spawnPos.y = - i * innerRadius;
    //     for (int j = 0; j < zAmount; j++){
    //         spawnPos.z = j * circleHeight - zCenterOff;
    //         float xOffset = (j + i) % 2 == 0 ? 0.0f : innerRadius;
    //         for (int k = 0; k < xAmount; k++){
    //             spawnPos.x = k * diameter + xOffset - xCenterOff;
    //             positions.emplace_back(spawnPos);
    //         }
    //     }
    // }

    // cubic model, a lot of space exist
    // for (int i = 0; i < yAmount; i++){
    //     spawnPos.y = - i * circleHeight;
    //     float xzOffset = i % 2 == 0 ? 0.0f : innerRadius;
    //     for (int j = 0; j < zAmount; j++){
    //         spawnPos.z = j * diameter + xzOffset;
    //         for (int k = 0; k < xAmount; k++){
    //             spawnPos.x = k * diameter + xzOffset;
    //             positions.emplace_back(spawnPos);
    //         }
    //     }
    // }


    // best model for good height so far
    // for (int i = 0; i < yAmount; i++){
    //     spawnPos.y = - i * circleHeight;
    //     float zOffset = (i % 3)*circleHeight/3;

    //     for (int j = 0; j < zAmount; j++){
    //         spawnPos.z = j * circleHeight + zOffset;
    //         float xOffset = (j + i) % 2 == 0 ? 0.0f : innerRadius;
    //         for (int k = 0; k < xAmount; k++){
    //             spawnPos.x = k * diameter + xOffset;
    //             positions.emplace_back(spawnPos);
    //         }
    //     }
    // }


    // generate sphere object's data
    VertexArray sphere;
    sphere.GenVertexBuffer(sizeof(s_quad_verts), (void*)s_quad_verts, 4, GL_STATIC_DRAW);
    sphere.InsertLayout(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    sphere.GenIndexBuffer(sizeof(s_quad_inds), (void*)s_quad_inds, IndexType::UINT8, GL_STATIC_DRAW);

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
    Shader skyboxShader("res/shaders/skybox.vert", "res/shaders/skybox.frag");


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

        // // draw model
        // shader.Use();
        
        // shader.SetUniformMatrix4fv("u_Projection", projection);
        // shader.SetUniformMatrix4fv("u_View", view);
        // shader.SetUniform3fv("u_CameraPos", cam.m_Position);

        // // render the loaded model
        // glm::mat4 model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
        // model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
        // shader.SetUniformMatrix4fv("u_Model", model);
        // ourModel.Draw();
        

        // draw quads
        quadShader.Use();
        quadShader.SetUniform1f("u_FarDist", far_distance);
        quadShader.SetUniformMatrix4fv("u_Projection", projection);
        quadShader.SetUniformMatrix4fv("u_View", view);
        quadShader.SetUniform3fv("u_CameraPos", cam.m_Position);
        
        //quadShader.SetUniform3fv("u_DirLight.ambient", 0.3f * glm::vec3(1.0f));
        //quadShader.SetUniform3fv("u_DirLight.diffuse",  0.5f * glm::vec3(1.0f));
        //quadShader.SetUniform3fv("u_DirLight.specular", 0.5f * glm::vec3(1.0f));

        quadShader.SetUniform3fv("u_Color", glm::vec3(0.5f, 0.8f, 1.0f));
        quadShader.SetUniform1i("u_Texture", 0);
        sphereTex.BindTo(1);
        skyboxTex.BindTo(0);
        // draw quad

        sphere.Bind();
        for (int i = 0; i < positions.size()/2; i++){
            // world transformation
            float scaleF;
            glm::mat4 model = lookToCamera(positions[i], outerRadius, scaleF);

            quadShader.SetUniform1f("u_ScaleFactor", scaleF);
            quadShader.SetUniform3fv("u_Center", positions[i]);
            quadShader.SetUniformMatrix4fv("u_Model", model);
            // render the light cubes
            glDrawElements(GL_TRIANGLES, sphere.m_IndxBuffer.Count(), sphere.m_IndxBuffer.GetType(), 0);
        }

        quadShader.SetUniform3fv("u_Color", glm::vec3(0.8f, 0.8f, 1.0f));
        for (int i = positions.size()/2; i < positions.size(); i++){
            // world transformation
            float scaleF;
            glm::mat4 model = lookToCamera(positions[i], outerRadius, scaleF);

            quadShader.SetUniform1f("u_ScaleFactor", scaleF);
            quadShader.SetUniform3fv("u_Center", positions[i]);
            quadShader.SetUniformMatrix4fv("u_Model", model);
            // render the light cubes
            glDrawElements(GL_TRIANGLES, sphere.m_IndxBuffer.Count(), sphere.m_IndxBuffer.GetType(), 0);
        }

        // // draw lighting objects
        // cubeMesh.m_Shaders[0].m_Shader.Use();
        // cubeMesh.LoadTextures(0);
        // cubeMesh.m_Shaders[0].m_Shader.SetUniformMatrix4fv("u_Projection", projection);
        // cubeMesh.m_Shaders[0].m_Shader.SetUniformMatrix4fv("u_View", view);
        // cubeMesh.m_Shaders[0].m_Shader.SetUniform3fv("u_CameraPos", cam.m_Position);

        // // update spotlight properties
        // cubeMesh.m_Shaders[0].m_Shader.SetUniform3fv("u_SpotLight.position", cam.m_Position);
        // cubeMesh.m_Shaders[0].m_Shader.SetUniform3fv("u_SpotLight.direction", cam.m_Front);


        // for (int i = 0; i < 1; i++){
        //     // calculate the model matrix for each object and pass it to shader before drawing
        //     glm::mat4 model = glm::mat4(1.0f);
        //     model = glm::translate(model, cubePositions[i]);
        //     float angle = 20.0f * i;
        //     model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

        //     cubeMesh.m_Shaders[0].m_Shader.SetUniformMatrix4fv("u_Model", model);
        //     // render the cube
        //     cubeMesh.Draw();
        // }


        // // draw last objs with different shader
        // cubeMesh.m_Shaders[2].m_Shader.Use();
        // cubeMesh.LoadTextures(2);
        // cubeMesh.m_Shaders[2].m_Shader.SetUniformMatrix4fv("u_Projection", projection);
        // cubeMesh.m_Shaders[2].m_Shader.SetUniformMatrix4fv("u_View", view);
        // cubeMesh.m_Shaders[2].m_Shader.SetUniform3fv("u_CameraPos", cam.m_Position);

        // // update spotlight properties
        // cubeMesh.m_Shaders[2].m_Shader.SetUniform3fv("u_SpotLight.position", cam.m_Position);
        // cubeMesh.m_Shaders[2].m_Shader.SetUniform3fv("u_SpotLight.direction", cam.m_Front);
        
        // for (int i = 5; i < 10; i++){
        //     // calculate the model matrix for each object and pass it to shader before drawing
        //     glm::mat4 model = glm::mat4(1.0f);
        //     model = glm::translate(model, cubePositions[i]);
        //     float angle = 20.0f * i;
        //     model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

        //     cubeMesh.m_Shaders[0].m_Shader.SetUniformMatrix4fv("u_Model", model);
        //     // render the cube
        //     cubeMesh.Draw();
        // }

        // // draw light objects
        // cubeMesh.m_Shaders[1].m_Shader.Use();
        // cubeMesh.m_Shaders[1].m_Shader.SetUniformMatrix4fv("u_Projection", projection);
        // cubeMesh.m_Shaders[1].m_Shader.SetUniformMatrix4fv("u_View", view);

        // for (int i = 0; i < 4; i++){
        //     // world transformation
        //     glm::mat4 model = glm::mat4(1.0f);
        //     model = glm::translate(model, pointLightPositions[i]);
        //     //model = glm::scale(model, glm::vec3(0.2f));
        //     cubeMesh.m_Shaders[1].m_Shader.SetUniformMatrix4fv("u_Model", model);
            
        //     // render the light cubes
        //     cubeMesh.Draw();
        // }

        // draw skybox
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
    glm::vec3 right = glm::normalize(glm::cross(m_MainCamera->m_WorldUp, direction));
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