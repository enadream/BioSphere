#pragma once

#include "physics/bound_box.hpp"
#include "world/sphere.hpp"
#include "world/config.hpp"

#include <array>
#include <cstdint>
#include <vector>
#include <string>

struct ChunkCoordinates {
    int32_t Z, X;

    ChunkCoordinates() = default;
    ChunkCoordinates(int32_t x_, int32_t z_) : Z(z_), X(x_) {}

    bool operator == (const ChunkCoordinates& other) const { return X == other.X && Z == other.Z; }
    
    // rows are Z columns are X axis
    bool operator < (const ChunkCoordinates& other){
        if (Z == other.Z) {
            return X < other.X;
        } else {
            return Z < other.Z;
        }
    }

    // returns concatenated 64 bit key Z|X
    uint64_t GetKey() const noexcept {
        uint64_t key = static_cast<uint64_t>(static_cast<uint32_t>(Z)) << 32;
        key |= static_cast<uint64_t>(static_cast<uint32_t>(X));
        return key;
    }
    // sets X and Z from key
    void SetFromKey(uint64_t key){
        Z = static_cast<int32_t>(key >> 32);
        X = static_cast<int32_t>(key & 0xFFFFFFFF);
    }
};

struct GPUBufferInfo { // Holds the info about where this chunk located in the gpu buffer
    uint32_t StartOffset = 0;
    uint32_t Count = 0;
};

// All LOD levels for one chunk, stored back-to-back in a single buffer.
// data layout: [LOD0 spheres][LOD1 spheres][LOD2 spheres][LOD3 spheres]
struct ChunkLODSet {
    std::vector<CompactSphere>          data;
    std::array<uint32_t, LOD_LEVELS>    lodOffsets{};
    std::array<uint32_t, LOD_LEVELS>    lodCounts{};
};

// --- Chunk Class ---
class Chunk {
public:
    Chunk(int32_t x, int32_t z);

    // 1.Destructor
    ~Chunk() = default;
    // 2.Copy Constructor
    Chunk(const Chunk&) = delete;
    // 3.Copy Assignment Operator
    Chunk& operator=(const Chunk&) = delete;

    // 4.Move Constructor
    Chunk(Chunk&& other);
    // 5.Move Assignment Operator
    //Chunk& operator=(Chunk&& other);


    // Core Data
    bool AddSphere(const Sphere& sphere);
    bool RemoveSphere(const SpherePosition targetPos);
    
    // Find range of spheres in a specific cell (x, z)
    bool GetCell(uint8_t x, uint8_t z, uint32_t &outOffset, uint32_t &outSize) const;

    // accomplish binary search and if find the sphere returns true, else false and update outIndex
    bool BinarySearch(const SpherePosition target, uint32_t& outIndex) const ;

    // Debugging
    void ToString(std::string& outStr) const;

    // GPU Generation
    // maxError = 0  -> full detail (preserves AO/Lights).
    // maxError > 0  -> adaptive quadtree: blocks are merged until height variance <= maxError.
    //                 Flat areas collapse aggressively; mountains stay fine-grained.
    // maxError = 0, maxBlockSize = CHUNK_SIZE -> full detail (preserves AO/Lights, includes all layers).
    // maxError > 0 or maxBlockSize < CHUNK_SIZE -> adaptive quadtree on surface heights only.
    //   maxBlockSize caps how large a merged block can be, preventing over-sized spheres near the player.
    void GenerateMesh(std::vector<GPUSphere>& outBuffer, float sphereRadius,
                      float maxError = 0.0f, uint8_t maxBlockSize = CHUNK_SIZE) const;

    // Calculates bound boxes
    void CalculateBounds();

    // Builds all LOD_LEVELS compact LOD meshes from current sphere data.
    // Idempotent. Call after generation/deserialization, before the renderer needs LODs.
    void GenerateLODs();

    // Getters
    const BoundBox& GetBounds() const { return m_Bounds; }
    BoundBox& GetBounds() { return m_Bounds; } // Mutable accessor for updates

    uint32_t GetSize() const {return m_Spheres.size();}
    const ChunkCoordinates& GetCoordinates() const { return m_Coordinates; }
    std::vector<Sphere>& GetSpheres() { return m_Spheres; }
    const std::vector<Sphere>& GetSpheres() const { return m_Spheres; }
    const ChunkLODSet& GetLODs() const { return m_LODs; }

    // Serializer
    void Serialize(std::vector<uint8_t>& buffer);
    void Deserialize(const std::vector<uint8_t>& buffer, uint64_t offset);

public:
    GPUBufferInfo GPUInfo; // Public for Renderer access
    bool IsDirty = false; // indicates that the chunk has updated so it needs to be saved to disk

private:
    // Helper to keep spheres sorted by LocalIndex then Height for binary search
    void SortSpheres();

private:
    ChunkCoordinates m_Coordinates;
    BoundBox m_Bounds;
    std::vector<Sphere> m_Spheres;
    ChunkLODSet m_LODs;
};