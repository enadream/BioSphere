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
    Camera(glm::vec3 position, float zNear, float zFar, int32_t width, int32_t height, float fov_y = 45.0f, 
        glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = -30.0f);

    void ProcessMovement(Camera_Movement direction, float speed, float delta_time);
    void ProcessMouseScroll(float y_offset);
    void Rotate(float yaw_offset, float pitch_offset);

    void CalculateFrustum();
    
    // view and projection matrix
    inline glm::mat4 GetViewMatrix() const {
        return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
    }
    inline glm::mat4 GetProjMatrix() const {
        return glm::perspective(GetFovYRad(), GetAspectRatio(), m_Near, m_Far);
    }
    inline glm::mat4 GetProjViewMat() const {
        return GetProjMatrix() * GetViewMatrix();
    }

    // vectors
    inline glm::vec3 GetPosition() const {
        return m_Position;
    }
    inline void SetPositionX(float x){
        m_Position.x = x;
    }
    inline void SetPositionY(float y){
        m_Position.y = y;
    }
    inline void SetPositionZ(float z){
        m_Position.z = z;
    }
    inline void SetPosition(glm::vec3 pos){
        m_Position = pos;
    }
    inline glm::vec3 GetUp() const {
        return m_Up;
    }
    inline glm::vec3 GetFront() const {
        return m_Front;
    }
    inline glm::vec3 GetRight() const {
        return m_Right;
    }

    // Near Z value
    inline float GetNear() const {
        return m_Near;
    }
    // Far Z value
    inline float GetFar() const {
        return m_Far;
    }
    // returns the fovY in degrees
    inline float GetFovYDeg() const {
        return m_FovY;
    }
    // returns the fovY in radians
    inline float GetFovYRad() const {
        return glm::radians(m_FovY);
    }
    // returns screen width pixel amount
    inline uint32_t GetWidth() const {
        return m_Width;
    }
    // returns screen height pixel amount
    inline uint32_t GetHeight() const {
        return m_Height;
    }
    inline float GetAspectRatio() const {
        return (float)m_Width/(float)m_Height;
    }
    inline void SetWidthHeight(uint32_t width, uint32_t height) {
        m_Width = width;
        m_Height = height;
    }
    
public: // variables
    Frustum m_Frustum;
    
private: // variables
    // camera vectors
    glm::vec3 m_Position;   // Camera position in world space
    glm::vec3 m_Up;         // Up vector of the camera
    glm::vec3 m_Front;      // Forward vector of the camera
    glm::vec3 m_Right;      // Right vector of the camera
    float m_Near;
    float m_Far;

    // screen infos
    float m_FovY;
    int32_t m_Width;
    int32_t m_Height;
    float m_Yaw; // horizontal axis in degrees
    float m_Pitch; // vertical axis in degrees

    glm::vec3 m_WorldUp;    // World up direction

private: // functions
    void updateCameraVectors();
};

#endif