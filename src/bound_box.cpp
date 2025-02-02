#include "bound_box.hpp"

BoundBox::BoundBox(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max) :
    minX(x_min), maxX(x_max), minY(y_min), maxY(y_max), minZ(z_min), maxZ(z_max) {}


bool BoundBox::IsInFrustumScalar(Frustum &frust) {
    for (const auto& plane : frust.planes) {
            const float nx = plane.normal.x;
            const float ny = plane.normal.y;
            const float nz = plane.normal.z;
            const float d = plane.distance;

            // Select p-vertex
            const float px = nx >= 0 ? maxX : minX;
            const float py = ny >= 0 ? maxY : minY;
            const float pz = nz >= 0 ? maxZ : minZ;

            // Compute distance
            const float dist = nx * px + ny * py + nz * pz + d;
            if(dist < 0) return false;
    }
    return true;
}

bool BoundBox::IsInFrustum(Frustum& frust) {
    // Preload box bounds into registers
    const __m128 min = _mm_setr_ps(minX, minY, minZ, 0);
    const __m128 max = _mm_setr_ps(maxX, maxY, maxZ, 0);
    const __m128 zero = _mm_setzero_ps();

    for(const Plane& plane : frust.planes) {
        // Load plane normal and distance
        const __m128 normal = _mm_loadu_ps(&plane.normal.x);
        const __m128 plane_d = _mm_set1_ps(plane.distance);
        
        // Compare normal components to zero
        const __m128 cmp = _mm_cmpge_ps(normal, zero);
        // Select min/max based on comparison
        const __m128 p_vertex = _mm_blendv_ps(min, max, cmp);

        // Calculate dot product (normal · p_vertex) + distance
        __m128 dp = _mm_mul_ps(p_vertex, normal);
        dp = _mm_hadd_ps(dp, dp);
        dp = _mm_hadd_ps(dp, dp);
        
        // Add plane distance
        const __m128 result = _mm_add_ss(dp, plane_d);
        
        if(_mm_comilt_ss(result, zero)) 
            return false;
    }
    return true;
}

void BoundBox::Print() {
    printf("Bound Box: X: %f, %f | Y: %f, %f | Z: %f, %f \n", minX, maxX, minY, maxY, minZ, maxZ);
}