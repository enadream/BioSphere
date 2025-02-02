#include "camera.hpp"

Camera::Camera(glm::vec3 position, float zNear, float zFar, int32_t width, int32_t height, glm::vec3 up, float yaw, float pitch) : 
    m_Position(position), m_Near(zNear), m_Far(zFar), m_Width(width), m_Height(height), m_WorldUp(up), m_Yaw(yaw), m_Pitch(pitch) {

    // front is calculated in update camera vector
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
}

void Camera::CalculateFrustum() {
    // 1. Direct matrix construction (avoid lookAt/perspective overhead)
    const float aspect = static_cast<float>(m_Width) / m_Height;
    const float tanHalfFOV = tan(glm::radians(m_Fov * 0.5f));
    const float f = 1.0f / tanHalfFOV;

    // Optimized projection matrix (perspective)
    glm::mat4 projection(0.0f);
    projection[0][0] = f / aspect;
    projection[1][1] = f;
    projection[2][2] = (m_Far + m_Near) / (m_Near - m_Far);
    projection[2][3] = -1.0f;
    projection[3][2] = (2.0f * m_Far * m_Near) / (m_Near - m_Far);

    // Optimized view matrix using precomputed basis vectors
    glm::mat4 view(1.0f);
    view[0][0] = m_Right.x; view[1][0] = m_Right.y; view[2][0] = m_Right.z;
    view[0][1] = m_Up.x;    view[1][1] = m_Up.y;    view[2][1] = m_Up.z;
    view[0][2] = -m_Front.x; view[1][2] = -m_Front.y; view[2][2] = -m_Front.z;
    view[3][0] = -glm::dot(m_Right, m_Position);
    view[3][1] = -glm::dot(m_Up, m_Position);
    view[3][2] = glm::dot(m_Front, m_Position);

    // 2. Matrix multiplication (column-major optimization)
    const glm::mat4 clipMatrix = projection * view;

    // 3. Plane extraction with SIMD-friendly layout
    const float* m = &clipMatrix[0][0];

    // Left plane (x = 0)
    m_Frustum.planes[0].normal.x = m[3] + m[0];
    m_Frustum.planes[0].normal.y = m[7] + m[4];
    m_Frustum.planes[0].normal.z = m[11] + m[8];
    m_Frustum.planes[0].distance = m[15] + m[12];

    // Right plane (x = 1)
    m_Frustum.planes[1].normal.x = m[3] - m[0];
    m_Frustum.planes[1].normal.y = m[7] - m[4];
    m_Frustum.planes[1].normal.z = m[11] - m[8];
    m_Frustum.planes[1].distance = m[15] - m[12];

    // Bottom plane (y = 0)
    m_Frustum.planes[2].normal.x = m[3] + m[1];
    m_Frustum.planes[2].normal.y = m[7] + m[5];
    m_Frustum.planes[2].normal.z = m[11] + m[9];
    m_Frustum.planes[2].distance = m[15] + m[13];

    // Top plane (y = 1)
    m_Frustum.planes[3].normal.x = m[3] - m[1];
    m_Frustum.planes[3].normal.y = m[7] - m[5];
    m_Frustum.planes[3].normal.z = m[11] - m[9];
    m_Frustum.planes[3].distance = m[15] - m[13];

    // Near plane (z = 0)
    m_Frustum.planes[4].normal.x = m[3] + m[2];
    m_Frustum.planes[4].normal.y = m[7] + m[6];
    m_Frustum.planes[4].normal.z = m[11] + m[10];
    m_Frustum.planes[4].distance = m[15] + m[14];

    // Far plane (z = 1)
    m_Frustum.planes[5].normal.x = m[3] - m[2];
    m_Frustum.planes[5].normal.y = m[7] - m[6];
    m_Frustum.planes[5].normal.z = m[11] - m[10];
    m_Frustum.planes[5].distance = m[15] - m[14];

    // 4. Fast normalization using reciprocal square root
    for(Plane& plane : m_Frustum.planes) {
        const float invLength = 1.0f / glm::sqrt(
            plane.normal.x * plane.normal.x +
            plane.normal.y * plane.normal.y +
            plane.normal.z * plane.normal.z
        );
        
        plane.normal *= invLength;
        plane.distance *= invLength;
    }
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