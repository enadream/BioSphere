#pragma once

#include "physics/bound_box.hpp"

#include <array>
#include <cstdint>
#include <vector>
#include <string>
#include <glm/glm.hpp>

// Constants
// Represents the width and breadth of chunk size
constexpr uint8_t CHUNK_SIZE = 16;
constexpr float SPHERE_RADIUS = 0.5f;

// --- Data Structures ---

struct LightVector { // RGB value of a light vector
    uint8_t R, G, B;
    LightVector() : R(0), G(0), B(0) {}
    LightVector(uint8_t r, uint8_t g, uint8_t b) : R(r), G(g), B(b) {}
};

struct SpherePosition {
    int16_t DiscreteHeight = 0;
    uint16_t CellIndex = 0;    // CellIndex = z * CHUNK_SIZE + x

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

struct SphereData { // CPU representation
    uint16_t ChunkTypeAndFlags = 0;
    uint16_t AmbientOcclusion = 0;
    std::array<LightVector, 6> Lights;  // 6 direction +- (x,y,z)
    SpherePosition Position;    // local position data of the sphere

    SphereData(uint8_t x, int16_t y, uint8_t z) : Position(x, y ,z){}
    
    void ToString(std::string& outStr) const;
};

// GPU Layout (std140/std430 compatible if needed)
struct GPUSphere {
    glm::vec4 PositionRadius; // xyz = pos, w = radius
    uint16_t ChunkTypeAndFlags;  // ChunkTypeAndFlags
    uint16_t AmbientOcclusion;   // AmbientOcclusion
    std::array<LightVector, 6> Lights; // 6 directional light
    uint16_t Padding; // Align to 4 bytes
};
static_assert(sizeof(GPUSphere) == 40, "GPUSphere size mismatch");

struct ChunkCoordinates {
    int32_t X, Z;
    ChunkCoordinates(int32_t x_, int32_t z_) : X(x_), Z(z_) {}
    bool operator==(const ChunkCoordinates& other) const { return X == other.X && Z == other.Z; }
};

struct GPUBufferInfo { // Holds the info about where this chunk located in the gpu buffer
    uint32_t StartOffset = 0;
    uint32_t Count = 0;
};

// --- Chunk Class ---
class Chunk {
public:
    Chunk(int32_t x, int32_t z);

    // Core Data
    bool AddSphere(const SphereData& sphere);
    bool RemoveSphere(const SpherePosition targetPos);
    
    // Find range of spheres in a specific cell (x, z)
    bool GetCell(uint8_t x, uint8_t z, uint32_t &outOffset, uint32_t &outSize) const;

    // accomplish binary search and if find the sphere returns true, else false and update outIndex
    bool BinarySearch(const SpherePosition target, uint32_t& outIndex) const ;

    // Debugging
    void ToString(std::string& outStr) const;

    // GPU Generation
    void GenerateMesh(std::vector<GPUSphere>& outBuffer, float sphereRadius) const;

    // Getters
    const BoundBox& GetBounds() const { return m_Bounds; }
    BoundBox& GetBounds() { return m_Bounds; } // Mutable accessor for updates
    
    uint32_t GetSize() const {return m_Spheres.size();}
    const ChunkCoordinates& GetCoordinates() const { return m_Coordinates; }
    std::vector<SphereData>& GetSpheres() { return m_Spheres; }

public:
    GPUBufferInfo GPUInfo; // Public for Renderer access

private:
    // Helper to keep spheres sorted by LocalIndex then Height for binary search
    void SortSpheres();

private:
    ChunkCoordinates m_Coordinates;
    BoundBox m_Bounds;
    std::vector<SphereData> m_Spheres;
};