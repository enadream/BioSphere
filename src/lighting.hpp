#include "bound_box.hpp"
#include <glm/glm.hpp>

struct LightProjection {
    glm::mat4 projMatrix;
    glm::mat4 viewMatrix;
    glm::vec3 lightPosition;
    int textureWidth;
    int textureHeight;

    inline glm::mat4 GetProjViewMat(){
        return projMatrix * viewMatrix;
    }
};

LightProjection CalcLightProperties(
    const BoundBox &scene_AABB, 
    const glm::vec3 &light_direction,
    float sphere_radius,
    int target_pixel_size);

