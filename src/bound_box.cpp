#include "bound_box.hpp"

BoundBox::BoundBox(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max) : m_Min(x_min, y_min, z_min),
    m_Max(x_max, y_max, z_max) {}

bool BoundBox::IsOnFrustum(const Frustum &frustum){
    for (uint32_t i = 0; i < frustum.planes.size(); i++){
        const glm::vec4 &g = frustum.planes[i];
        if ((glm::dot(g, glm::vec4(m_Min.x, m_Min.y, m_Min.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(m_Max.x, m_Min.y, m_Min.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(m_Min.x, m_Max.y, m_Min.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(m_Max.x, m_Max.y, m_Min.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(m_Min.x, m_Min.y, m_Max.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(m_Max.x, m_Min.y, m_Max.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(m_Min.x, m_Max.y, m_Max.z, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(m_Max.x, m_Max.y, m_Max.z, 1.0f)) < 0.0)) 
        {
            // Not visible - all returned negative
            return false;
        }
    }
    // Potentially visible
    return true;
}

void BoundBox::Print() {
    printf("Bound Box: X: %f, %f | Y: %f, %f | Z: %f, %f \n", m_Min.x, m_Max.x, m_Min.y, m_Max.y, m_Min.z, m_Max.z);
}

glm::vec3 BoundBox::GetCenter() {
    glm::vec3 result;

    result.x = (m_Min.x + m_Max.x) / 2.0f;
    result.y = (m_Min.y + m_Max.y) / 2.0f;
    result.z = (m_Min.z + m_Max.z) / 2.0f;

    return result;
}

glm::vec3 BoundBox::GetSize() {
    glm::vec3 result;
    result.x = m_Max.x - m_Min.x;
    result.y = m_Max.y - m_Min.y;
    result.z = m_Max.z - m_Min.z;

    return result;
}
