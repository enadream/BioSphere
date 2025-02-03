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
    glm::vec3 GetCenter();
    glm::vec3 GetSize();

public: // variables
    float minX, maxX; // X-axis bounds
    float minY, maxY; // Y-axis bounds
    float minZ, maxZ; // Z-axis bounds

private: // variables

private: // functions

};


#endif