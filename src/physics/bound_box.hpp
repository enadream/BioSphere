#pragma once

#include "physics/frustum.hpp"
#include <glm/glm.hpp>

class BoundBox {
public:
    BoundBox() = default;
    BoundBox(const glm::vec3& min, const glm::vec3& max);
    BoundBox(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max);

    // Checks if the box is inside or intersecting the frustum
    bool IsOnFrustum(const Frustum& frustum) const;

    // Getters
    glm::vec3 GetCenter() const { return (m_Min + m_Max) * 0.5f; }
    glm::vec3 GetSize() const { return m_Max - m_Min; } // No abs needed if min < max
    glm::vec3 GetMin() const { return m_Min; }
    glm::vec3 GetMax() const { return m_Max; }

    // Debugging
    void Print() const;

private:
    // Helper to get the corner of the box "most in the direction" of the normal
    glm::vec3 GetPositiveVertex(const glm::vec3& normal) const;
    // Helper to get the corner "most opposite" to the normal
    glm::vec3 GetNegativeVertex(const glm::vec3& normal) const;

public:
    glm::vec3 m_Min;
    glm::vec3 m_Max;
};