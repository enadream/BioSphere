#include "physics/bound_box.hpp"
#include "core/log.hpp"

BoundBox::BoundBox(const glm::vec3& min, const glm::vec3& max) 
    : m_Min(min), m_Max(max) 
{
}

BoundBox::BoundBox(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max)
    : m_Min(x_min, y_min, z_min), m_Max(x_max, y_max, z_max)
{
}

bool BoundBox::IsOnFrustum(const Frustum& frustum) const {
    // Check box against all 6 planes
    for (const auto& plane : frustum.Planes) {
        // Find the point on the box most aligned with the plane normal
        glm::vec3 pVertex = GetPositiveVertex(plane.Normal);

        // If that point is behind the plane, the whole box is outside
        // Plane equation: dot(N, P) + D = distance
        // If distance < 0, it's outside
        if (plane.GetSignedDistance(pVertex) < 0.0f) {
            return false;
        }
    }
    return true;
}

glm::vec3 BoundBox::GetPositiveVertex(const glm::vec3& normal) const {
    glm::vec3 p = m_Min;
    if (normal.x >= 0) p.x = m_Max.x;
    if (normal.y >= 0) p.y = m_Max.y;
    if (normal.z >= 0) p.z = m_Max.z;
    return p;
}

glm::vec3 BoundBox::GetNegativeVertex(const glm::vec3& normal) const {
    glm::vec3 n = m_Max;
    if (normal.x >= 0) n.x = m_Min.x;
    if (normal.y >= 0) n.y = m_Min.y;
    if (normal.z >= 0) n.z = m_Min.z;
    return n;
}

void BoundBox::Print() const {
    LOG_INFO("AABB [Min: (%.2f, %.2f, %.2f), Max: (%.2f, %.2f, %.2f)]", 
             m_Min.x, m_Min.y, m_Min.z, m_Max.x, m_Max.y, m_Max.z);
}