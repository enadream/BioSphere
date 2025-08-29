#ifndef BOUND_BOX_HPP
#define BOUND_BOX_HPP

#include <glm/glm.hpp>
#include <immintrin.h>

#include "frustum.hpp"
#include "camera.hpp"

class BoundBox { // 24 bytes
public: // functions
    BoundBox() = default;
    BoundBox(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max);

    // SIMD-optimized version (SSE4.1)
    //bool IsInFrustum(const Camera& camera);
    bool IsOnFrustum(const Frustum &frustum);

    void Print();
    inline glm::vec3 GetCenter() const {
        return (m_Min + m_Max) * 0.5f;
    }
    inline glm::vec3 GetSize() const {
        return glm::abs(m_Max - m_Min);
    }

    // Get the 8 corners of the AABB
    inline std::vector<glm::vec4> GetCorners() const {
        return {
            {m_Min.x, m_Min.y, m_Min.z, 1.0f},
            {m_Max.x, m_Min.y, m_Min.z, 1.0f},
            {m_Min.x, m_Max.y, m_Min.z, 1.0f},
            {m_Max.x, m_Max.y, m_Min.z, 1.0f},
            {m_Min.x, m_Min.y, m_Max.z, 1.0f},
            {m_Max.x, m_Min.y, m_Max.z, 1.0f},
            {m_Min.x, m_Max.y, m_Max.z, 1.0f},
            {m_Max.x, m_Max.y, m_Max.z, 1.0f}
        };
    }

public: // variables
    glm::vec3 m_Min;
    glm::vec3 m_Max;

    // float minX, maxX; // X-axis bounds
    // float minY, maxY; // Y-axis bounds
    // float minZ, maxZ; // Z-axis bounds

private: // variables

private: // functions

};


#endif