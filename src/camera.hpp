#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glad/glad.h>
// glmath
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "frustum.hpp"

enum Camera_Movement : uint8_t {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
public:
    Camera(glm::vec3 position, float zNear, float zFar, int32_t width, int32_t height, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
        float yaw = -90.0f, float pitch = -30.0f);

    glm::mat4 GetViewMatrix() const;
    void ProcessMovement(Camera_Movement direction, float speed, float delta_time);
    void ProcessMouseScroll(float y_offset);
    void Rotate(float yaw_offset, float pitch_offset);

    Frustum GetFrustum();
    void CalculateFrustum();
    
public: // variables
    glm::vec3 m_Position;   // Camera position in world space
    glm::vec3 m_Front;      // Forward vector of the camera
    glm::vec3 m_WorldUp;    // World up direction
    float m_Fov = 45.0f;
    int32_t m_Width; // w/h
    int32_t m_Height;
    Frustum m_Frustum;
    
private: // variables
    float m_Near;
    float m_Far;
    float m_Yaw; // horizontal axis
    float m_Pitch; // vertical axis
    glm::vec3 m_Up;         // Up vector of the camera
    glm::vec3 m_Right;      // Right vector of the camera

    

private: // functions
    void updateCameraVectors();
};

#endif