#include "camera.hpp"

Camera::Camera(glm::vec3 position, float zNear, float zFar, int32_t width, int32_t height, float fow_y, glm::vec3 world_up, float yaw, float pitch) : 
    m_Position(position), m_Near(zNear), m_Far(zFar), m_Width(width), m_Height(height), m_FovY(fow_y), m_WorldUp(world_up),  m_Yaw(yaw), m_Pitch(pitch) {

    // front is calculated in update camera vector
    updateCameraVectors();
}

void Camera::CalculateFrustum() {
    const glm::mat4 pv = GetProjViewMat();
    const glm::mat4 tpv = glm::transpose(pv);

    m_Frustum.planes = { 
        // left, right, bottom, top
        (tpv[3] + tpv[0]),
        (tpv[3] - tpv[0]),
        (tpv[3] + tpv[1]),
        (tpv[3] - tpv[1]),
        // near, far
        (tpv[3] + tpv[2]),
        (tpv[3] - tpv[2])};
}


// void Camera::CalculateFrustum() {
//     // Obtain the combined view-projection matrix
//     glm::mat4 projView = GetProjViewMat();
//     // Transpose the matrix to access rows easily
//     glm::mat4 transposed = glm::transpose(projView);

//     // Extract each plane's coefficients by combining rows of the transposed matrix
//     std::array<glm::vec4, 6> planes;
//     planes[0] = transposed[3] + transposed[0]; // Left plane
//     planes[1] = transposed[3] - transposed[0]; // Right plane
//     planes[2] = transposed[3] + transposed[1]; // Bottom plane
//     planes[3] = transposed[3] - transposed[1]; // Top plane
//     planes[4] = transposed[3] + transposed[2]; // Near plane
//     planes[5] = transposed[3] - transposed[2]; // Far plane

//     // Normalize each plane and store in the frustum
//     for (size_t i = 0; i < 6; ++i) {
//         const glm::vec4& planeCoeff = planes[i];
//         glm::vec3 normal(planeCoeff.x, planeCoeff.y, planeCoeff.z);
//         float length = glm::length(normal);

//         if (length < 1e-6f) {
//             // Handle invalid plane (unlikely but possible)
//             continue;
//         }

//         // Normalize the coefficients
//         normal /= length;
//         float distance = -planeCoeff.w / length;

//         // Assign the calculated plane to the frustum
//         m_Frustum.planes[i] = Plane(normal, distance);
//     }
// }

// void Camera::CalculateFrustum() {
//     const float halfVside = m_Far * tanf(GetFovYRad());
//     const float halfHside = halfVside * ((float)(m_Width)/(float)m_Height);
    
//     const glm::vec3 frontMultipFar = m_Far * m_Front;

//     // near face
//     m_Frustum.planes[0] = {m_Front, m_Position + m_Near * m_Front};
//     // far face
//     m_Frustum.planes[1] = {-m_Front, m_Position + frontMultipFar};
//     // right face
//     m_Frustum.planes[2] = {glm::cross(frontMultipFar - halfHside * m_Right, m_Up), m_Position};
//     // left face
//     m_Frustum.planes[3] = {glm::cross(m_Up, frontMultipFar + halfHside * m_Right), m_Position};
//     // top face
//     m_Frustum.planes[4] = {glm::cross(m_Right, frontMultipFar - halfVside * m_Up), m_Position};
//     // bottom face
//     m_Frustum.planes[5] = {glm::cross(frontMultipFar + halfVside * m_Up, m_Right), m_Position};

// }

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
    m_FovY -= y_offset;
    if (m_FovY < 1.0f)
        m_FovY = 1.0f;
    if (m_FovY > 60.0f)
        m_FovY = 60.0f;
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



// void Camera::CalculateFrustum() {
//     const float aspect = static_cast<float>(m_Width) / m_Height;
    
//     // 1. Rebuild camera basis vectors to ensure orthonormality
//     m_Front = glm::normalize(glm::vec3{
//         cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch)),
//         sin(glm::radians(m_Pitch)),
//         sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch))
//     });
    
//     m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
//     m_Up = glm::normalize(glm::cross(m_Right, m_Front));

//     // 2. Create view-projection matrix with position compensation
//     const glm::mat4 view = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
//     const glm::mat4 proj = glm::perspective(
//         glm::radians(m_Fov), aspect, m_Near, m_Far
//     );
    
//     const glm::mat4 clip = proj * view;

//     // 3. Plane extraction with position-aware normalization
//     auto extract_plane = [&](int row, bool negative) {
//         glm::vec4 coeffs;
//         coeffs.x = clip[0][3] + (negative ? -clip[0][row] : clip[0][row]);
//         coeffs.y = clip[1][3] + (negative ? -clip[1][row] : clip[1][row]);
//         coeffs.z = clip[2][3] + (negative ? -clip[2][row] : clip[2][row]);
//         coeffs.w = clip[3][3] + (negative ? -clip[3][row] : clip[3][row]);

//         const float len = glm::length(glm::vec3(coeffs));
//         return Plane{
//             glm::vec3(coeffs) / len,  // Normal
//             coeffs.w / len            // Distance
//         };
//     };

//     m_Frustum.planes[0] = extract_plane(0, true);   // Left
//     m_Frustum.planes[1] = extract_plane(0, false);  // Right
//     m_Frustum.planes[2] = extract_plane(1, true);   // Bottom
//     m_Frustum.planes[3] = extract_plane(1, false);  // Top
//     m_Frustum.planes[4] = extract_plane(2, true);   // Near
//     m_Frustum.planes[5] = extract_plane(2, false);  // Far
// }

// void Camera::CalculateFrustum() {
//     // 1. Direct matrix construction (avoid lookAt/perspective overhead)
//     const float aspect = static_cast<float>(m_Width) / m_Height;
//     const float tanHalfFOV = tan(glm::radians(m_Fov * 0.5f));
//     const float f = 1.0f / tanHalfFOV;

//     // Optimized projection matrix (perspective)
//     glm::mat4 projection(0.0f);
//     projection[0][0] = f / aspect;
//     projection[1][1] = f;
//     projection[2][2] = (m_Far + m_Near) / (m_Near - m_Far);
//     projection[2][3] = -1.0f;
//     projection[3][2] = (2.0f * m_Far * m_Near) / (m_Near - m_Far);

//     // Optimized view matrix using precomputed basis vectors
//     glm::mat4 view(1.0f);
//     view[0][0] = m_Right.x; view[1][0] = m_Right.y; view[2][0] = m_Right.z;
//     view[0][1] = m_Up.x;    view[1][1] = m_Up.y;    view[2][1] = m_Up.z;
//     view[0][2] = -m_Front.x; view[1][2] = -m_Front.y; view[2][2] = -m_Front.z;
//     view[3][0] = -glm::dot(m_Right, m_Position);
//     view[3][1] = -glm::dot(m_Up, m_Position);
//     view[3][2] = glm::dot(m_Front, m_Position);

//     // 2. Matrix multiplication (column-major optimization)
//     const glm::mat4 clipMatrix = projection * view;

//     // 3. Plane extraction with SIMD-friendly layout
//     const float* m = &clipMatrix[0][0];

//     // Left plane (x = 0)
//     m_Frustum.planes[0].normal.x = m[3] + m[0];
//     m_Frustum.planes[0].normal.y = m[7] + m[4];
//     m_Frustum.planes[0].normal.z = m[11] + m[8];
//     m_Frustum.planes[0].distance = m[15] + m[12];

//     // Right plane (x = 1)
//     m_Frustum.planes[1].normal.x = m[3] - m[0];
//     m_Frustum.planes[1].normal.y = m[7] - m[4];
//     m_Frustum.planes[1].normal.z = m[11] - m[8];
//     m_Frustum.planes[1].distance = m[15] - m[12];

//     // Bottom plane (y = 0)
//     m_Frustum.planes[2].normal.x = m[3] + m[1];
//     m_Frustum.planes[2].normal.y = m[7] + m[5];
//     m_Frustum.planes[2].normal.z = m[11] + m[9];
//     m_Frustum.planes[2].distance = m[15] + m[13];

//     // Top plane (y = 1)
//     m_Frustum.planes[3].normal.x = m[3] - m[1];
//     m_Frustum.planes[3].normal.y = m[7] - m[5];
//     m_Frustum.planes[3].normal.z = m[11] - m[9];
//     m_Frustum.planes[3].distance = m[15] - m[13];

//     // Near plane (z = 0)
//     m_Frustum.planes[4].normal.x = m[3] + m[2];
//     m_Frustum.planes[4].normal.y = m[7] + m[6];
//     m_Frustum.planes[4].normal.z = m[11] + m[10];
//     m_Frustum.planes[4].distance = m[15] + m[14];

//     // Far plane (z = 1)
//     m_Frustum.planes[5].normal.x = m[3] - m[2];
//     m_Frustum.planes[5].normal.y = m[7] - m[6];
//     m_Frustum.planes[5].normal.z = m[11] - m[10];
//     m_Frustum.planes[5].distance = m[15] - m[14];

//     // 4. Fast normalization using reciprocal square root
//     for(Plane& plane : m_Frustum.planes) {
//         const float invLength = 1.0f / glm::sqrt(
//             plane.normal.x * plane.normal.x +
//             plane.normal.y * plane.normal.y +
//             plane.normal.z * plane.normal.z
//         );
        
//         plane.normal *= invLength;
//         plane.distance *= invLength;
//     }
// }