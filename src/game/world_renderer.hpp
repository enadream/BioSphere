#pragma once

#include "renderer/shader.hpp"
#include "renderer/gl_buffer.hpp"
#include "renderer/camera.hpp"
#include "util/memory_manager.hpp"
#include "world/chunk.hpp"

#include <glad/glad.h>
#include <cstdint>
#include <memory>
#include <queue>
#include <vector>
#include <unordered_map>

#ifndef GL_PARAMETER_BUFFER
#define GL_PARAMETER_BUFFER 0x80EE
#endif

class WorldHandler;

// HQ pipeline ChunkInfo - matches frustum_culler.comp std430 layout.
struct ChunkInfo {
    uint32_t offset;
    uint32_t size;
    int32_t  posX, posZ;
    float    bbMinX, bbMinY, bbMinZ;
    float    bbMaxX, bbMaxY, bbMaxZ;
};
static_assert(sizeof(ChunkInfo) == 40, "ChunkInfo size mismatch with shader");

// LO pipeline ChunkInfo - matches frustum_culler_lo.comp / atom_compact.vert std430 layout.
struct ChunkInfoLO {
    int32_t  posX;
    int32_t  posZ;
    float    bbMinY;
    float    bbMaxY;
    uint32_t lodOffsets[LOD_LEVELS];
    uint32_t lodCounts[LOD_LEVELS];
};
static_assert(sizeof(ChunkInfoLO) == 48, "ChunkInfoLO size mismatch with shader");

// Dual-pipeline GPU renderer.
//
// HQ pipeline (full 40-byte GPUSphere):
//   - Active only for chunks inside HQ_RENDER_RANGE of the player.
//   - Uploaded from CPU chunk data on demand; evicted when chunk leaves the range.
//   - Uses frustum_culler.comp + atom.vert/frag.
//
// LO pipeline (compact 8-byte CompactSphere, multi-LOD):
//   - Active for all chunks in the load ring, including those also in HQ.
//   - One slab per chunk holds all LOD_LEVELS pre-computed levels back-to-back.
//   - Culler skips chunks inside HQ_RENDER_RANGE and picks the LOD slice per
//     distance ring.
//   - Uses frustum_culler_lo.comp + atom_compact.vert/frag.
class WorldRenderer {
public:
    WorldRenderer() = default;
    ~WorldRenderer();

    WorldRenderer(const WorldRenderer&)            = delete;
    WorldRenderer& operator=(const WorldRenderer&) = delete;

    void Init(uint32_t renderDist);

    // Process the world's recently-arrived delta and (on center change) scan slot
    // maps for evictions / HQ promotions. Cheap - actual GL writes happen in UpdateUploads.
    // Non-const because it consumes the world's arrived-list delta.
    void SyncChunks(WorldHandler& world);

    // Drain up to CHUNK_UPLOAD_PER_FRAME entries from each pending queue (nearest first).
    void UpdateUploads(const WorldHandler& world);

    void Render(const Camera& cam);

    uint32_t GetSphereCount() const noexcept;

    void Shutdown();

private:
    void     SetupVAO_HQ();
    void     SetupVAO_LO();
    uint32_t EstimateMaxSpheresHQ() const;
    uint32_t EstimateMaxSpheresLO() const;
    bool     GrowVBO_HQ();
    bool     GrowVBO_LO();
    void     RenderHQ(const Camera& cam);
    void     RenderLO(const Camera& cam);

    bool     UploadHQ(const Chunk& chunk);
    bool     UploadLO(const Chunk& chunk);
    void     EvictHQ(uint64_t key);
    void     EvictLO(uint64_t key);

    static bool InSquare(ChunkCoordinates a, ChunkCoordinates b, int32_t dist) noexcept;

    // --- Shaders
    std::unique_ptr<Shader>  m_CullerShader_HQ;
    std::unique_ptr<Shader>  m_CullerShader_LO;
    std::unique_ptr<Shader>  m_AtomShader_HQ;
    std::unique_ptr<Shader>  m_AtomShader_LO;

    // --- HQ pipeline buffers
    GLBuffer  m_SphereVBO_HQ;
    GLBuffer  m_ChunkInfoSSBO_HQ;
    GLBuffer  m_DrawCmdSSBO_HQ;
    GLBuffer  m_VisibleCountBuf_HQ;
    GLuint    m_VAO_HQ = 0;
    std::unique_ptr<MemoryManager>  m_HQMem;
    std::vector<ChunkInfo>          m_ChunkInfoCPU_HQ;

    struct HQEntry {
        uint32_t slot;       // index in m_ChunkInfoCPU_HQ
        uint32_t vboOffset;
        uint32_t vboSize;
    };
    std::unordered_map<uint64_t, HQEntry> m_HQSlot;

    // --- LO pipeline buffers
    GLBuffer  m_SphereVBO_LO;
    GLBuffer  m_ChunkInfoSSBO_LO;
    GLBuffer  m_DrawCmdSSBO_LO;
    GLBuffer  m_VisibleCountBuf_LO;
    GLBuffer  m_VisibleChunkIdxSSBO_LO;
    GLuint    m_VAO_LO = 0;
    std::unique_ptr<MemoryManager>  m_LOMem;
    std::vector<ChunkInfoLO>        m_ChunkInfoCPU_LO;

    struct LOEntry {
        uint32_t slot;          // index in m_ChunkInfoCPU_LO
        uint32_t vboOffset;     // base of slab
        uint32_t vboTotalSize;  // sum of all LOD counts in the slab
    };
    std::unordered_map<uint64_t, LOEntry> m_LOSlot;

    // --- Pending uploads (nearest first)
    struct PendingUpload {
        uint64_t         key;
        ChunkCoordinates coords;
        float            distSq;
        bool operator<(const PendingUpload& o) const noexcept { return distSq > o.distSq; }
    };
    std::priority_queue<PendingUpload> m_PendingHQ;
    std::priority_queue<PendingUpload> m_PendingLO;

    // --- State
    uint32_t m_RenderDist     = 0;
    uint32_t m_LoadDist       = 0;
    uint32_t m_HqLoadRange    = 0;  // HQ_RENDER_RANGE * HQ_LOAD_FACTOR, rounded up
    uint32_t m_MaxChunks      = 0;
    uint32_t m_MaxSpheresHQ   = 0;
    uint32_t m_MaxSpheresLO   = 0;
    ChunkCoordinates m_CamChunk{0, 0};
    ChunkCoordinates m_LastSyncCenter{INT32_MAX, INT32_MAX};
    bool     m_Initialized      = false;
    bool     m_ChunkInfoHQDirty = false;
    bool     m_ChunkInfoLODirty = false;
};
