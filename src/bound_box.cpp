#include "bound_box.hpp"

BoundBox::BoundBox(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max) :
    minX(x_min), maxX(x_max), minY(y_min), maxY(y_max), minZ(z_min), maxZ(z_max) {}

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
