#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include <array>
#include <glm/glm.hpp>

#include "stdio.h"

struct Plane {
    // unit vector frustum normal
    glm::vec3 normal;

    // distance from origin to the nearest point in the plane
    float distance;

    Plane() = default;
    Plane(const glm::vec3& norm, const float dist) : normal(norm), distance(dist) { }
    Plane(const glm::vec3& norm, const glm::vec3& p1) : normal(glm::normalize(norm)), distance(glm::dot(normal, p1)) { }

    float GetSignedDistance(const glm::vec3 &point) const {
        return glm::dot(normal, point) - distance;
    }
};

struct Frustum {
    std::array<glm::vec4, 6> planes;

    // void Print(){
    //     for (int i = 0; i < planes.size(); i++){
    //         printf("normal : %f, %f, %f dist: %f\n", planes[i].normal.x, planes[i].normal.y, planes[i].normal.z, planes[i].distance);
    //     }
    // }
};

#endif