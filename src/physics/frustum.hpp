#pragma once

#include "glm/geometric.hpp"
#include "core/log.hpp"

#include <array>
#include <glm/glm.hpp>


// Helper struct for individual plane math
// Equation: Ax + By + Cz + D = 0
struct Plane {
    // Unit vector frustum normal
    glm::vec3 Normal; // (A, B, C)
    // Distance from origin to the nearest point in the plane
    float Distance; // D

    Plane() = default;
    // Constructor for manual definition
    Plane(const glm::vec3& norm, const float dist) : Normal(norm), Distance(dist) { }

    // Constructor for Camera extraction (converts vec4 directly)
    // vec4 = (nx, ny, nz, d)
    Plane(const glm::vec4& equation)
        : Normal(equation.x, equation.y, equation.z), Distance(equation.w) {}

    // Construct from Normal and a Point on the plane
    // Equation: N * P + D = 0  =>  D = -N * P
    Plane(const glm::vec3& norm, const glm::vec3& p1) { 
        Normal = glm::normalize(norm);
        Distance = -glm::dot(Normal, p1);
    }
    
    // Normalize the plane so the normal vector has length 1
    void Normalize() {
        float len = glm::length(Normal);
        if (len > 0.0f){
            float invLen = 1.0f / len;
            Normal *= invLen;
            Distance *= invLen;
        }
    }

    // Assumes Plane is normalized. 
    // Returns > 0 if point is in front (normal direction), < 0 if behind.
    float GetSignedDistance(const glm::vec3 &point) const {
        // Standard Plane Equation: Ax + By + Cz + D
        return glm::dot(Normal, point) - Distance;
    }
};

struct Frustum {
    // Stored as vec4: x,y,z = Normal, w = Distance (D)
    // Left, Right, Bottom, Top, Near, Far
    std::array<Plane, 6> Planes;

    // This is required because Gribb-Hartmann extraction produces non-normalized planes.
    // Normalize all planes in the frustum
    void Normalize() {
        for (auto& plane : Planes) {
            plane.Normalize();
        }
    }

    // Helper to get raw pointer for glUniform4fv
    // This assumes sizeof(Plane) == sizeof(vec4) == 16 bytes, which is standard.
    const float* GetData() const {
        return &Planes[0].Normal.x;
    }

    void Print() const {
        for (auto& plane : Planes){
            LOG_INFO("Normal : %f, %f, %f Dist: %f\n", plane.Normal.x, plane.Normal.y, plane.Normal.z, plane.Distance);
        }
    }
};

