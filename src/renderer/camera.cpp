#include "renderer/camera.hpp"
#include <algorithm> // for clamp

Camera::Camera(glm::vec3 position, 
    int32_t width, int32_t height, 
    float yaw, float pitch) 
    : m_Position(position),
      m_Front(0.0f, 0.0f, -1.0f),
      m_Up(0.0f, 1.0f, 0.0f),
      m_Right(1.0f, 0.0f, 0.0f),
      m_WorldUp(0.0f, 1.0f, 0.0f),
      m_Width(width),
      m_Height(height),
      m_Yaw(yaw),
      m_Pitch(pitch) 
{
    Update();
}

void Camera::ProcessMovement(CameraMovement direction, float deltaTime){
    float velocity = Settings.MovementSpeed * deltaTime;

    switch (direction) {
        case CameraMovement::Forward:  m_Position += m_Front * velocity; break;
        case CameraMovement::Backward: m_Position -= m_Front * velocity; break;
        case CameraMovement::Left:     m_Position -= m_Right * velocity; break;
        case CameraMovement::Right:    m_Position += m_Right * velocity; break;
        case CameraMovement::Up:       m_Position += m_Up * velocity; break;
        case CameraMovement::Down:     m_Position -= m_Up * velocity; break;
    }
}

void Camera::Rotate(float yawOffset, float pitchOffset){
    yawOffset *= Settings.MouseSensitivity;
    pitchOffset *= Settings.MouseSensitivity;

    m_Yaw   += yawOffset;
    m_Pitch += pitchOffset;

    if (Settings.ConstrainPitch) {
        m_Pitch = std::clamp(m_Pitch, -Settings.PitchLimit, Settings.PitchLimit);
    }
}

void Camera::ProcessMouseScroll(float yOffset) {
    Settings.FovY -= yOffset * Settings.ZoomSensitivity;
    Settings.FovY = std::clamp(Settings.FovY, 1.0f, 90.0f);
}

void Camera::CalculateFrustum() {
    // 1. Extract Planes (Gribb-Hartmann)
    // Transpose to get rows easily
    const glm::mat4 tpv = glm::transpose(m_ProjViewMatrix);

    m_Frustum.Planes = { 
        (tpv[3] + tpv[0]), // Left
        (tpv[3] - tpv[0]), // Right
        (tpv[3] + tpv[1]), // Bottom
        (tpv[3] - tpv[1]), // Top
        (tpv[3] + tpv[2]), // Near
        (tpv[3] - tpv[2])  // Far
    };

    // 2. Normalize Planes (Using the new helper method in Frustum struct)
    m_Frustum.Normalize();
}

glm::mat4 Camera::GetViewMatrix() const noexcept {
    return m_ViewMatrix;
}
glm::mat4 Camera::GetProjMatrix() const noexcept {
    return m_ProjMatrix;
}
glm::mat4 Camera::GetProjViewMatrix() const noexcept {
    return m_ProjViewMatrix;
}

void Camera::SetViewportSize(int32_t width, int32_t height) noexcept {
    m_Width = width;
    m_Height = height;
}

void Camera::Update(){
    // 1. Calculate Front Vector
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);

    // 2. Re-calculate Right and Up
    // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up    = glm::normalize(glm::cross(m_Right, m_Front));

    // 3. Update Matrices
    m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
    m_ProjMatrix = glm::perspective(GetFovYRad(), GetAspectRatio(), Settings.NearClip, Settings.FarClip);
    m_ProjViewMatrix = m_ProjMatrix * m_ViewMatrix;
}