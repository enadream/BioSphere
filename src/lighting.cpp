#include "lighting.hpp"

LightProjection CalcLightProperties(
    const BoundBox &scene_AABB, 
    const glm::vec3 &light_direction,
    float sphere_radius,
    int target_pixel_size)
{
    LightProjection result;

    // --- Step 1: Determine the Light's View Matrix ---
    
    // The light should look at the center of the scene AABB
    glm::vec3 aabbCenter = scene_AABB.GetCenter();

    // Place the camera far enough away along the light's direction
    float aabbDiagonal = glm::length(scene_AABB.m_Max - scene_AABB.m_Min);
    result.lightPosition = aabbCenter - light_direction * (aabbDiagonal * 0.5f);

    // Determine the "up" vector for the lookAt matrix
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    // Edge case: if light is pointing straight up/down, the cross product will be zero.
    // In this case, we use a different up vector.
    if (glm::abs(glm::dot(light_direction, upVector)) > 0.999f) {
        upVector = glm::vec3(0.0f, 0.0f, 1.0f); // Use Z-axis as up
    }

    result.viewMatrix = glm::lookAt(result.lightPosition, aabbCenter, upVector);

    // --- Step 2: Find the Bounding Box in Light Space ---
    auto corners = scene_AABB.GetCorners();

    // Transform all 8 corners into light's view space
    for (auto& corner : corners) {
        corner = result.viewMatrix * corner;
    }

    // Find the min and max extents of the transformed corners
    glm::vec3 minLightSpace = glm::vec3(corners[0]);
    glm::vec3 maxLightSpace = glm::vec3(corners[0]);

    for (size_t i = 1; i < corners.size(); ++i) {
        minLightSpace.x = std::min(minLightSpace.x, corners[i].x);
        minLightSpace.y = std::min(minLightSpace.y, corners[i].y);
        minLightSpace.z = std::min(minLightSpace.z, corners[i].z);

        maxLightSpace.x = std::max(maxLightSpace.x, corners[i].x);
        maxLightSpace.y = std::max(maxLightSpace.y, corners[i].y);
        maxLightSpace.z = std::max(maxLightSpace.z, corners[i].z);
    }


    // --- Step 3: Define the Orthographic Projection Matrix ---
    // Use the light-space extents to create a tight projection
    // Note the sign flip for near/far planes as they are distances from the camera
    float left = minLightSpace.x;
    float right = maxLightSpace.x;
    float bottom = minLightSpace.y;
    float top = maxLightSpace.y;
    float nearPlane = -maxLightSpace.z;
    float farPlane = -minLightSpace.z;

    result.projMatrix = glm::ortho(left, right, bottom, top, nearPlane, farPlane);

    // --- Step 4: Calculate the Output Texture Size ---

    float orthoWidth = right - left;
    float orthoHeight = top - bottom;

    // Calculate how many world units should be covered by a single pixel
    float sphereWorldDiameter = sphere_radius * 2.0f;
    float worldUnitsPerPixel = sphereWorldDiameter / static_cast<float>(target_pixel_size);

    // Calculate the texture dimensions based on this density
    result.textureWidth = static_cast<int>(std::ceil(orthoWidth / worldUnitsPerPixel));
    result.textureHeight = static_cast<int>(std::ceil(orthoHeight / worldUnitsPerPixel));

    return result;
}