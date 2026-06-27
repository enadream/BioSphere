#include "world/chunk.hpp"
#include "core/log.hpp"
#include "glm/fwd.hpp"
#include "physics/bound_box.hpp"
#include "world/sphere.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <string>
#include <format>

Chunk::Chunk(int32_t x, int32_t z) : m_Coordinates({x, z})
{  }

// move data
Chunk::Chunk(Chunk&& other){
    GPUInfo = other.GPUInfo;
    IsDirty = other.IsDirty;

    m_Coordinates = other.m_Coordinates;
    m_Bounds = other.m_Bounds;
    m_Spheres = std::move(other.m_Spheres);
    m_LODs = std::move(other.m_LODs);
}

bool Chunk::AddSphere(const Sphere& sphere) {
    // Insert while maintaining order (based on LocalIndex) for binary search/locality
    uint32_t insertIndex;

    if (BinarySearch(sphere.Position, insertIndex)){
        LOG_WARN("[CHUNK] Sphere at position already exists, skipping add.");
        return false;
    }

    m_Spheres.emplace(m_Spheres.begin()+insertIndex, sphere);

    // Update bound box
    float worldY = (float)sphere.Position.DiscreteHeight * SPHERE_RADIUS;
    m_Bounds.m_Min.y = std::min(worldY-SPHERE_RADIUS, m_Bounds.m_Min.y);
    m_Bounds.m_Max.y = std::max(worldY+SPHERE_RADIUS, m_Bounds.m_Max.y);

    IsDirty = true;
    return true;
}

bool Chunk::RemoveSphere(const SpherePosition targetPos) {
    uint32_t posIndex;

    if (!BinarySearch(targetPos, posIndex)){
        LOG_WARN("[CHUNK] Sphere at position does not exist, skipping remove.");
        return false;
    }

    m_Spheres.erase(m_Spheres.begin()+posIndex);

    IsDirty = true;
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

void Chunk::GenerateMesh(std::vector<GPUSphere>& outBuffer, float radius, float maxError, uint8_t maxBlockSize) const {
    const float chunkWorldX = m_Coordinates.X * CHUNK_SIZE * radius;
    const float chunkWorldZ = m_Coordinates.Z * CHUNK_SIZE * radius;

    // Full-detail fast path - preserves per-sphere AO and light data, emits ALL layers.
    // Only taken when no LOD constraints are active.
    if (maxError <= 0.0f && maxBlockSize >= CHUNK_SIZE) {
        outBuffer.reserve(outBuffer.size() + m_Spheres.size());
        for (const auto& sphere : m_Spheres) {
            GPUSphere gpu;
            uint8_t lx, lz; int16_t ly;
            sphere.Position.GetCoordinates(lx, ly, lz);
            gpu.PositionRadius    = glm::vec4(chunkWorldX + float(lx) * radius,
                                              float(ly) * radius,
                                              chunkWorldZ + float(lz) * radius,
                                              radius);
            gpu.ChunkTypeAndFlags = sphere.ChunkTypeAndFlags;
            gpu.AmbientOcclusion  = sphere.AmbientOcclusion;
            gpu.Lights            = sphere.Lights;
            outBuffer.emplace_back(gpu);
        }
        return;
    }

    // Adaptive LOD path.
    // Build per-cell surface height (world-space float, highest sphere wins).
    constexpr uint32_t NCELLS  = CHUNK_SIZE * CHUNK_SIZE;
    constexpr float    EMPTY   = std::numeric_limits<float>::lowest();
    std::array<float, NCELLS> cellH;
    cellH.fill(EMPTY);
    for (const auto& sphere : m_Spheres) {
        const uint16_t ci = sphere.Position.CellIndex;
        if (ci < NCELLS) {
            const float h = float(sphere.Position.DiscreteHeight) * radius;
            if (h > cellH[ci]) cellH[ci] = h;
        }
    }

    // Iterative DFS quadtree subdivision.
    // Each block is tested: if height variance <= maxError (or size==1), emit one sphere.
    // Otherwise split into four equal sub-blocks and recurse.
    // Max stack depth for a 16x16 chunk with 4 subdivision levels: ~13 entries.
    struct Block { uint8_t bx, bz, size; };
    std::array<Block, 32> stack;
    int32_t top = 0;
    stack[top++] = {0, 0, CHUNK_SIZE};

    while (top > 0) {
        const auto [bx, bz, size] = stack[--top];

        float hMin = std::numeric_limits<float>::max();
        float hMax = EMPTY;
        float hSum = 0.0f;
        uint32_t cnt = 0;
        for (uint32_t dz = 0; dz < size; ++dz) {
            for (uint32_t dx = 0; dx < size; ++dx) {
                const float h = cellH[(bz + dz) * CHUNK_SIZE + (bx + dx)];
                if (h != EMPTY) {
                    if (h < hMin) hMin = h;
                    if (h > hMax) hMax = h;
                    hSum += h;
                    ++cnt;
                }
            }
        }
        if (cnt == 0) continue;

        // Subdivide if block exceeds the size cap OR variance exceeds budget.
        if (size > 1 && (size > maxBlockSize || (hMax - hMin) > maxError)) {
            const uint8_t half = size / 2;
            stack[top++] = {bx,              bz,              half};
            stack[top++] = {uint8_t(bx+half),bz,              half};
            stack[top++] = {bx,              uint8_t(bz+half),half};
            stack[top++] = {uint8_t(bx+half),uint8_t(bz+half),half};
            continue;
        }

        // Emit one sphere for this block.
        // Center is the geometric middle of the block's cell grid.
        // Use average height - gives correct biome colour and smooth visual impression.
        GPUSphere gpu{};
        gpu.PositionRadius = glm::vec4(
            chunkWorldX + (float(bx) + float(size - 1) * 0.5f) * radius,
            hSum / float(cnt),
            chunkWorldZ + (float(bz) + float(size - 1) * 0.5f) * radius,
            radius * float(size));
        outBuffer.emplace_back(gpu);
    }
}

void Chunk::GenerateLODs() {
    constexpr uint32_t NCELLS = static_cast<uint32_t>(CHUNK_SIZE) * static_cast<uint32_t>(CHUNK_SIZE);
    constexpr float    EMPTY  = std::numeric_limits<float>::lowest();

    std::array<float, NCELLS> cellH;
    cellH.fill(EMPTY);
    for (const auto& sphere : m_Spheres) {
        const uint16_t ci = sphere.Position.CellIndex;
        if (ci < NCELLS) {
            const float h = float(sphere.Position.DiscreteHeight) * SPHERE_RADIUS;
            if (h > cellH[ci]) cellH[ci] = h;
        }
    }

    m_LODs.data.clear();
    m_LODs.lodOffsets.fill(0);
    m_LODs.lodCounts.fill(0);

    for (uint32_t lod = 0; lod < LOD_LEVELS; ++lod) {
        const uint32_t blockSize    = 1u << lod;
        const uint32_t blocksPerRow = CHUNK_SIZE / blockSize;
        m_LODs.lodOffsets[lod] = static_cast<uint32_t>(m_LODs.data.size());

        uint32_t emitted = 0;
        for (uint32_t bz = 0; bz < blocksPerRow; ++bz) {
            for (uint32_t bx = 0; bx < blocksPerRow; ++bx) {
                float    sum = 0.0f;
                uint32_t cnt = 0;
                for (uint32_t dz = 0; dz < blockSize; ++dz) {
                    for (uint32_t dx = 0; dx < blockSize; ++dx) {
                        const uint32_t ci = (bz * blockSize + dz) * CHUNK_SIZE + (bx * blockSize + dx);
                        const float h = cellH[ci];
                        if (h != EMPTY) { sum += h; ++cnt; }
                    }
                }
                if (cnt == 0) continue;

                const int32_t halfRadX = static_cast<int32_t>(2u * bx * blockSize + blockSize - 1u);
                const int32_t halfRadZ = static_cast<int32_t>(2u * bz * blockSize + blockSize - 1u);
                const float   avgY     = sum / static_cast<float>(cnt);
                const int32_t halfRadY = static_cast<int32_t>(std::lround(2.0f * avgY / SPHERE_RADIUS));

                CompactSphere cs{};
                cs.lx      = static_cast<int16_t>(halfRadX);
                cs.ly      = static_cast<int16_t>(halfRadY);
                cs.lz      = static_cast<int16_t>(halfRadZ);
                cs.lodStep = static_cast<uint8_t>(blockSize);
                cs.color   = 0;
                m_LODs.data.push_back(cs);
                ++emitted;
            }
        }
        m_LODs.lodCounts[lod] = emitted;
    }
}

void Chunk::CalculateBounds(){
    const int32_t baseX = m_Coordinates.X * static_cast<int32_t>(CHUNK_SIZE);
    const int32_t baseZ = m_Coordinates.Z * static_cast<int32_t>(CHUNK_SIZE);
    m_Bounds.m_Min.x = SPHERE_RADIUS * static_cast<float>(baseX - 1);
    m_Bounds.m_Min.z = SPHERE_RADIUS * static_cast<float>(baseZ - 1);
    m_Bounds.m_Max.x = SPHERE_RADIUS * static_cast<float>(baseX + static_cast<int32_t>(CHUNK_SIZE));
    m_Bounds.m_Max.z = SPHERE_RADIUS * static_cast<float>(baseZ + static_cast<int32_t>(CHUNK_SIZE));

    m_Bounds.m_Min.y =  std::numeric_limits<float>::max();
    m_Bounds.m_Max.y = -std::numeric_limits<float>::max();
    for (const auto& sphere : m_Spheres) {
        float worldY = sphere.Position.DiscreteHeight * SPHERE_RADIUS;
        m_Bounds.m_Min.y = std::min(worldY - SPHERE_RADIUS, m_Bounds.m_Min.y);
        m_Bounds.m_Max.y = std::max(worldY + SPHERE_RADIUS, m_Bounds.m_Max.y);
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
        [](const Sphere& a, const Sphere& b) {
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

void Chunk::Serialize(std::vector<uint8_t>& buffer){
    // Key(8) | ChunkSize(4) | Payload(chunkSize bytes)
    // Payload = BBox(24) | SphereCount(4) | Spheres..

    // ChunkPosition(8) | chunkSize(4) | BBOX(24) | SphereCount(4) | Spheres...
    const uint32_t chunkSize = sizeof(BoundBox) + sizeof(uint32_t) + m_Spheres.size() * sizeof(Sphere);
    const uint32_t byteSize = sizeof(uint64_t) + sizeof(chunkSize) + chunkSize;
    
    // create sufficient space
    const size_t oldSize = buffer.size();
    buffer.resize(oldSize + byteSize);
    uint8_t *ptr = buffer.data();
    ptr += oldSize;
    
    // copy key
    const uint64_t key = m_Coordinates.GetKey();
    std::memcpy(ptr, &key, sizeof(key));
    ptr += sizeof(key);

    // copy chunkSize
    std::memcpy(ptr, &chunkSize, sizeof(chunkSize));
    ptr += sizeof(chunkSize);

    // copy boundbox, bound box contains vec3 min and max vectors
    std::memcpy(ptr, &m_Bounds, sizeof(m_Bounds));
    ptr += sizeof(m_Bounds);

    // copy sphere count
    const uint32_t sphereCount = static_cast<uint32_t>(m_Spheres.size());
    std::memcpy(ptr, &sphereCount, sizeof(sphereCount));
    ptr += sizeof(sphereCount);

    // copy spheres
    std::memcpy(ptr, m_Spheres.data(), m_Spheres.size() * sizeof(Sphere));
}

void Chunk::Deserialize(const std::vector<uint8_t>& buffer, uint64_t offset){
    // Key(8) | ChunkSize(4) | Payload(chunkSize bytes)
    // Payload = BBox(24) | SphereCount(4) | Spheres..

    const uint8_t *ptr = buffer.data();
    ptr += offset;

    // read key
    uint64_t key = 0;
    std::memcpy(&key, ptr, sizeof(key));
    ptr += sizeof(key);
    m_Coordinates.SetFromKey(key);

    // use chunk size to verify 
    uint32_t chunkSize = 0;
    std::memcpy(&chunkSize, ptr, sizeof(chunkSize));
    ptr += sizeof(chunkSize);

    if (sizeof(uint64_t) + sizeof(uint32_t) + chunkSize + offset > buffer.size()){
        LOG_ERROR("[CHUNK] Failed to deserialize, chunk size is bigger than buffer");
        return;
    }

    // read boundbox
    std::memcpy(&m_Bounds, ptr, sizeof(m_Bounds));
    ptr += sizeof(m_Bounds);

    // read sphere count
    uint32_t sphereCount = 0;
    std::memcpy(&sphereCount, ptr, sizeof(sphereCount));
    ptr += sizeof(sphereCount);

    // read spheres
    m_Spheres.resize(sphereCount);
    std::memcpy(m_Spheres.data(), ptr, sizeof(Sphere) * sphereCount);
}