#include "world/chunk.hpp"
#include "core/log.hpp"
#include <cstdint>
#include <iterator>
#include <string>
#include <format>

Chunk::Chunk(int32_t x, int32_t z) : m_Coordinates({x, z}) 
{  }

bool Chunk::AddSphere(const SphereData& sphere) {
    // Insert while maintaining order (based on LocalIndex) for binary search/locality
    uint32_t insertIndex;

    if (BinarySearch(sphere.Position, insertIndex)){
        LOG_WARN("[CHUNK] Sphere at position already exists, skipping add.");
        return false;
    }

    m_Spheres.emplace(m_Spheres.begin()+insertIndex, sphere);

    // Update bound box
    float worldY = (float)sphere.Position.DiscreteHeight * SPHERE_RADIUS;
    m_Bounds.m_Min.y = glm::min(worldY-SPHERE_RADIUS, m_Bounds.m_Min.y);
    m_Bounds.m_Max.y = glm::max(worldY+SPHERE_RADIUS, m_Bounds.m_Max.y);

    return true;
}

bool Chunk::RemoveSphere(const SpherePosition targetPos) {
    uint32_t posIndex;

    if (!BinarySearch(targetPos, posIndex)){
        LOG_WARN("[CHUNK] Sphere at position does not exist, skipping remove.");
        return false;
    }

    m_Spheres.erase(m_Spheres.begin()+posIndex);
    return true;
}

bool Chunk::GetCell(uint8_t x, uint8_t z, uint32_t &outOffset, uint32_t &outSize) const {
    const uint16_t targetIndex = static_cast<uint16_t>(z * CHUNK_SIZE + x);

    // -- Lower bound (first sphere with CellIndex == targetIndex) ----------------
    int64_t left = 0, right = m_Spheres.size();
    while (left < right){
        int64_t mid = (right - left)/2 + left;

        if (targetIndex > m_Spheres[mid].Position.CellIndex){
            // search in right half
            left = mid+1;
        }
        else { // when target <= mid
            right = mid;
        }
    }

    // validate lowerbound
    if (!(left < (int64_t)m_Spheres.size()) || m_Spheres[left].Position.CellIndex != targetIndex){
        LOG_WARN("[CHUNK] Target cell (%u, %u) not found.", x, z);
        return false;
    }

    outOffset = static_cast<uint32_t>(left); // start index is left

    // -- Upper bound (first sphere with CellIndex > targetIndex) -----------------
    right = m_Spheres.size();
    while (left < right){
        int64_t mid = (right-left)/2 + left;

        if (targetIndex < m_Spheres[mid].Position.CellIndex){
            // search in left half
            right = mid;
        }
        else { // when target >= mid
            left = mid+1;
        }
    }

   // left is now the exclusive upper bound
    outSize = static_cast<uint32_t>(left) - outOffset;
    return true;
}

void Chunk::GenerateMesh(std::vector<GPUSphere>& outBuffer, float radius) const {
    // Pre-calculate world position offsets based on chunk coordinates
    // We assume chunks are tightly packed.
    // If coords are (0,0), (1,0), etc., and CHUNK_SIZE is 16:
    // Chunk 0 starts at 0. Chunk 1 starts at 16 * radius.
    float chunkWorldX = m_Coordinates.X * CHUNK_SIZE * radius;
    float chunkWorldZ = m_Coordinates.Z * CHUNK_SIZE * radius;

    // reserve enough memory
    outBuffer.reserve(outBuffer.size()+m_Spheres.size());
    for (const auto& sphere : m_Spheres) {
        GPUSphere gpu;
        
        uint8_t localX, localZ;
        int16_t localY;
        sphere.Position.GetCoordinates(localX, localY, localZ);

        // Absolute World Position
        // Note: The logic in World::GenerateChunk used a similar calculation.
        // Ensure consistency between physics/logic bounds and visual position.
        float posX = chunkWorldX + ((float)localX * radius);
        float posZ = chunkWorldZ + ((float)localZ * radius);
        float posY = (float)localY * radius;

        gpu.PositionRadius = glm::vec4(posX, posY, posZ, radius);
        gpu.ChunkTypeAndFlags = sphere.ChunkTypeAndFlags;
        gpu.AmbientOcclusion = sphere.AmbientOcclusion;
        gpu.Lights = sphere.Lights;
        outBuffer.emplace_back(gpu);
    }
}

bool Chunk::BinarySearch(const SpherePosition targetPos, uint32_t& outIndex) const {
    int64_t left = 0;
    int64_t right = m_Spheres.size(); // right is not included in search

    while (left < right) {
        int64_t mid = (right-left)/2 + left;

        // check local index first
        if (targetPos == m_Spheres[mid].Position){
            outIndex = mid;
            return true;
        }
        else if (targetPos < m_Spheres[mid].Position){
            // search in left half
            right = mid;
        }
        else {
            left = mid+1;
        }
    }

    outIndex = left;
    return false;
}

void Chunk::SortSpheres() {
    std::sort(m_Spheres.begin(), m_Spheres.end(),
        [](const SphereData& a, const SphereData& b) {
            return a.Position < b.Position;
        });
}

void Chunk::ToString(std::string& outStr) const {
    std::format_to(std::back_inserter(outStr), "Chunk [{}, {}] - Total Spheres: {} ;\n", 
        m_Coordinates.X, m_Coordinates.Z, m_Spheres.size() );

    for (size_t i = 0; i < m_Spheres.size(); ++i) {
        std::format_to(std::back_inserter(outStr), "  {}: \n", i);
        m_Spheres[i].ToString(outStr);
    }
}

void SphereData::ToString(std::string& outStr) const {    
    // print position
    uint8_t x, z;
    int16_t y; 
    Position.GetCoordinates(x, y, z);
    std::format_to(std::back_inserter(outStr), "    Position: X:{}, Y:{}, Z:{}\n", x, y, z);
    // print chunk type and flags as binary
    std::format_to(std::back_inserter(outStr), "    Flags: {:b}\n", ChunkTypeAndFlags);
    // ambient as binary
    std::format_to(std::back_inserter(outStr), "    AmbientOcclusion: {:b}\n", AmbientOcclusion);
    // print light vectors
    for (uint32_t i = 0; i < Lights.size(); i++){
        std::format_to(std::back_inserter(outStr), "    LightVec[{}]: R:{}, G{}, B{}\n", i, Lights[i].R, Lights[i].G, Lights[i].B);
    }
}