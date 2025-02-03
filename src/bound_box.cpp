#include "bound_box.hpp"

BoundBox::BoundBox(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max) :
    minX(x_min), maxX(x_max), minY(y_min), maxY(y_max), minZ(z_min), maxZ(z_max) {}


// bool BoundBox::IsInFrustumScalar(Frustum &frust) {
//     for (const auto& plane : frust.planes) {
//             const float nx = plane.normal.x;
//             const float ny = plane.normal.y;
//             const float nz = plane.normal.z;
//             const float d = plane.distance;

//             // Select p-vertex
//             const float px = nx >= 0 ? maxX : minX;
//             const float py = ny >= 0 ? maxY : minY;
//             const float pz = nz >= 0 ? maxZ : minZ;

//             // Compute distance
//             const float dist = nx * px + ny * py + nz * pz + d;
//             if(dist < 0) return false;
//     }
//     return true;
// }

bool BoundBox::IsOnFrustum(const Frustum &frustum){
    for (uint32_t i = 0; i < frustum.planes.size(); i++){
        const glm::vec4 &g = frustum.planes[i];
        if ((glm::dot(g, glm::vec4(minX, minY, minZ, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(maxX, minY, minZ, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(minX, maxY, minZ, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(maxX, maxY, minZ, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(minX, minY, maxZ, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(maxX, minY, maxZ, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(minX, maxY, maxZ, 1.0f)) < 0.0) &&
            (glm::dot(g, glm::vec4(maxX, maxY, maxZ, 1.0f)) < 0.0)) 
        {
            // Not visible - all returned negative
            return false;
        }
    }
    // Potentially visible
    return true;
}

// bool BoundBox::IsOnFrustum(const Frustum &frustum){
//     for (uint32_t i = 0; i < frustum.planes.size(); i++){
//         if (!isOnOrForwardPlane(frustum.planes[i])){
//             return false;
//         }
//     }
//     return true;
// }

// bool BoundBox::IsInFrustum(const Camera& camera) {
//     const auto& frustum = camera.m_Frustum;
//     const glm::vec3 camPos = camera.m_Position;

//     // Early exit if box is behind camera
//     const float frontDist = glm::dot(camPos - glm::vec3(minX, minY, minZ), camera.m_Front);
//     if(frontDist > camera.m_Far) return false;

//     // Main plane tests
//     for(const auto& plane : frustum.planes) {
//         const glm::vec3 pVertex{
//             plane.normal.x >= 0 ? maxX : minX,
//             plane.normal.y >= 0 ? maxY : minY,
//             plane.normal.z >= 0 ? maxZ : minZ
//         };

//         if(glm::dot(plane.normal, pVertex) + plane.distance < 0)
//             return false;
//     }
//     return true;
// }

// bool BoundBox::IsInFrustum(Frustum& frust) {
//     // Preload box bounds into registers
//     const __m128 min = _mm_setr_ps(minX, minY, minZ, 0);
//     const __m128 max = _mm_setr_ps(maxX, maxY, maxZ, 0);
//     const __m128 zero = _mm_setzero_ps();

//     for(const Plane& plane : frust.planes) {
//         // Load plane normal and distance
//         const __m128 normal = _mm_loadu_ps(&plane.normal.x);
//         const __m128 plane_d = _mm_set1_ps(plane.distance);
        
//         // Compare normal components to zero
//         const __m128 cmp = _mm_cmpge_ps(normal, zero);
//         // Select min/max based on comparison
//         const __m128 p_vertex = _mm_blendv_ps(min, max, cmp);

//         // Calculate dot product (normal · p_vertex) + distance
//         __m128 dp = _mm_mul_ps(p_vertex, normal);
//         dp = _mm_hadd_ps(dp, dp);
//         dp = _mm_hadd_ps(dp, dp);
        
//         // Add plane distance
//         const __m128 result = _mm_add_ss(dp, plane_d);
        
//         if(_mm_comilt_ss(result, zero)) 
//             return false;
//     }
//     return true;
// }

void BoundBox::Print() {
    printf("Bound Box: X: %f, %f | Y: %f, %f | Z: %f, %f \n", minX, maxX, minY, maxY, minZ, maxZ);
}

glm::vec3 BoundBox::GetCenter() {
    glm::vec3 result;

    result.x = (minX + maxX) / 2.0f;
    result.y = (minY + maxY) / 2.0f;
    result.z = (minZ + maxZ) / 2.0f;

    return result;
}

glm::vec3 BoundBox::GetSize() {
    glm::vec3 result;
    result.x = maxX - minX;
    result.y = maxY - minY;
    result.z = maxZ - minZ;

    return result;
}
