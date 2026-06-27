#pragma once


#include "world/config.hpp"

#include <glm/glm.hpp>
#include <cstdint>
#include <array>
#include <string>


struct LightVector { // RGB value of a light vector
    uint8_t R, G, B;
    LightVector() : R(0), G(0), B(0) {}
    LightVector(uint8_t r, uint8_t g, uint8_t b) : R(r), G(g), B(b) {}
};

struct SpherePosition {
    int16_t DiscreteHeight = 0;
    uint16_t CellIndex = 0;    // CellIndex = z * CHUNK_SIZE + x

    SpherePosition() = default;
    
    SpherePosition(uint8_t x, int16_t y, uint8_t z) {
        CellIndex = static_cast<uint16_t>(z * CHUNK_SIZE + x);
        DiscreteHeight = y;
    }

    void GetCoordinates(uint8_t& outX, int16_t& outY, uint8_t& outZ) const {
        outY = DiscreteHeight;
        outZ = CellIndex / CHUNK_SIZE;
        outX = CellIndex % CHUNK_SIZE;
    }

    bool operator == (const SpherePosition& pos) const {
        return pos.CellIndex == CellIndex && pos.DiscreteHeight == DiscreteHeight;  
    }

    // Ascending order
    bool operator < (const SpherePosition& pos) const {
        if (CellIndex == pos.CellIndex){ // when their local index is same
            return DiscreteHeight < pos.DiscreteHeight;
        }
        else { // only compare local indexes
            return CellIndex < pos.CellIndex;
        }
    }
};

struct Sphere { // CPU representation
    uint16_t ChunkTypeAndFlags = 0;
    uint16_t AmbientOcclusion = 0;
    std::array<LightVector, 6> Lights;  // 6 direction +- (x,y,z)
    SpherePosition Position;    // local position data of the sphere

    Sphere() = default;
    Sphere(uint8_t x, int16_t y, uint8_t z) : Position(x, y ,z){}
    
    void ToString(std::string& outStr) const;
};
static_assert(std::is_trivially_copyable_v<Sphere>);


// GPU Layout (std140/std430 compatible if needed)
struct GPUSphere {
    glm::vec4 PositionRadius; // xyz = pos, w = radius
    uint16_t ChunkTypeAndFlags;  // ChunkTypeAndFlags
    uint16_t AmbientOcclusion;   // AmbientOcclusion
    std::array<LightVector, 6> Lights; // 6 directional light
    uint16_t Padding; // Align to 4 bytes
};
static_assert(sizeof(GPUSphere) == 40, "GPUSphere size mismatch");

// Compact LOD sphere uploaded once per chunk into the low-quality VBO.
// Position is chunk-local, in half-radius units so half-block offsets from
// even-sized LOD blocks fit exactly. World pos = chunkOrigin + xyz * (SPHERE_RADIUS * 0.5).
struct CompactSphere {
    int16_t lx, ly, lz;   // local x/y/z in half-radius units (y is absolute world y)
    uint8_t lodStep;      // block size 1 / 2 / 4 / 8 -> world radius = lodStep * SPHERE_RADIUS
    uint8_t color;        // palette index reserved for future material use
};
static_assert(sizeof(CompactSphere) == 8, "CompactSphere size mismatch");
static_assert(std::is_trivially_copyable_v<CompactSphere>);