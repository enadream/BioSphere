#include "camera.hpp"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : 
    m_Position(position), m_WorldUp(up), m_Yaw(yaw), m_Pitch(pitch) {
    
    m_Front = glm::vec3(0.0f, 0.0f, -1.0f);
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
}

void Camera::ProcessMovement(Camera_Movement direction, float speed, float delta_time){
    float deltaSpeed = speed * delta_time;

    switch (direction)
    {
    case FORWARD:
        m_Position += m_Front * deltaSpeed;
        break;
    case BACKWARD:
        m_Position -= m_Front * deltaSpeed;
        break;
    case LEFT:
        m_Position -= m_Right * deltaSpeed;
        break;
    case RIGHT:
        m_Position += m_Right * deltaSpeed;
        break;
    case UP:
        m_Position += m_Up * deltaSpeed;
        break;
    case DOWN:
        m_Position -= m_Up * deltaSpeed;
        break;
    }
}

void Camera::ProcessMouseScroll(float y_offset){
    m_Fov -= y_offset;
    if (m_Fov < 1.0f)
        m_Fov = 1.0f;
    if (m_Fov > 60.0f)
        m_Fov = 60.0f;
}

void Camera::Rotate(float yaw_offset, float pitch_offset){
    m_Yaw += yaw_offset;
    m_Pitch += pitch_offset;

    if (m_Pitch > 89.0f){
        m_Pitch = 89.0f;
    }
    if (m_Pitch < -89.0f){
        m_Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::updateCameraVectors(){
    // calculate new front
    glm::vec3 nFront;
    nFront.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    nFront.y = sin(glm::radians(m_Pitch));
    nFront.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(nFront);

    // also re-calculate the right and up vectors
    // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
} 