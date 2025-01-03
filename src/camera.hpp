#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glad/glad.h>
// glmath
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = -90.0f, float pitch = 0.0f);

    glm::mat4 GetViewMatrix() const;
    void ProcessMovement(Camera_Movement direction, float speed, float delta_time);
    void ProcessMouseScroll(float y_offset);
    void Rotate(float yaw_offset, float pitch_offset);


    float m_Fov = 45.0f;
public:
    glm::vec3 m_Position;   // Camera position in world space
    glm::vec3 m_Front;      // Forward vector of the camera
    
private:
    glm::vec3 m_Up;         // Up vector of the camera
    glm::vec3 m_Right;      // Right vector of the camera
    glm::vec3 m_WorldUp;    // World up direction

    float m_Yaw;
    float m_Pitch;

    void updateCameraVectors();
};

#endif