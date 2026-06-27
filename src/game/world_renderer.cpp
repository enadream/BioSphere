#include "game/world_renderer.hpp"
#include "world/world_handler.hpp"
#include "world/sphere.hpp"
#include "world/config.hpp"
#include "core/log.hpp"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

// --- Destructor

WorldRenderer::~WorldRenderer() {
    Shutdown();
}

// --- Init

void WorldRenderer::Init(uint32_t renderDist) {
    m_RenderDist  = renderDist;
    m_LoadDist    = static_cast<uint32_t>(std::ceil(renderDist * LOAD_DISTANCE_FACTOR));
    m_HqLoadRange = static_cast<uint32_t>(std::ceil(HQ_RENDER_RANGE * HQ_LOAD_FACTOR));
    m_MaxChunks   = (m_LoadDist * 2u + 1u) * (m_LoadDist * 2u + 1u);

    m_MaxSpheresHQ = EstimateMaxSpheresHQ();
    m_MaxSpheresLO = EstimateMaxSpheresLO();

    // Initial VBOs - moderate size, grow on demand. The HQ initial budget assumes
    // a baseline of ~CHUNK_SIZE^2 spheres per chunk (one per surface cell); steep
    // terrain that pushes past that triggers a growth.
    const uint32_t hqLoadChunks = (m_HqLoadRange * 2u + 1u) * (m_HqLoadRange * 2u + 1u);
    const uint32_t initHQ       = std::min(hqLoadChunks * static_cast<uint32_t>(CHUNK_SIZE) *
                                            static_cast<uint32_t>(CHUNK_SIZE), m_MaxSpheresHQ);
    const uint32_t loChunks     = std::min(m_MaxChunks, 4096u);
    const uint32_t initLO       = std::min(loChunks * 340u, m_MaxSpheresLO);

    LOG_INFO("[WorldRenderer] Init: renderDist=%u loadDist=%u maxChunks=%u hqLoadRange=%u",
             m_RenderDist, m_LoadDist, m_MaxChunks, m_HqLoadRange);
    LOG_INFO("[WorldRenderer] HQ VBO initial=%u (~%u MB) ceiling=%u (~%u MB)",
             initHQ, static_cast<uint32_t>(uint64_t(initHQ) * sizeof(GPUSphere) / (1024u * 1024u)),
             m_MaxSpheresHQ, static_cast<uint32_t>(uint64_t(m_MaxSpheresHQ) * sizeof(GPUSphere) / (1024u * 1024u)));
    LOG_INFO("[WorldRenderer] LO VBO initial=%u (~%u MB) ceiling=%u (~%u MB)",
             initLO, static_cast<uint32_t>(uint64_t(initLO) * sizeof(CompactSphere) / (1024u * 1024u)),
             m_MaxSpheresLO, static_cast<uint32_t>(uint64_t(m_MaxSpheresLO) * sizeof(CompactSphere) / (1024u * 1024u)));

    // --- HQ pipeline buffers (sized for the LOAD ring, not the render ring)
    m_SphereVBO_HQ        = GLBuffer(initHQ * static_cast<uint32_t>(sizeof(GPUSphere)), nullptr, GL_DYNAMIC_DRAW);
    m_HQMem               = std::make_unique<MemoryManager>(initHQ);
    m_ChunkInfoSSBO_HQ    = GLBuffer(hqLoadChunks * static_cast<uint32_t>(sizeof(ChunkInfo)), nullptr, GL_DYNAMIC_DRAW);
    m_DrawCmdSSBO_HQ      = GLBuffer(hqLoadChunks * 4u * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
    const uint32_t zero = 0;
    m_VisibleCountBuf_HQ  = GLBuffer(sizeof(uint32_t), &zero, GL_DYNAMIC_DRAW);
    m_ChunkInfoCPU_HQ.reserve(hqLoadChunks);
    SetupVAO_HQ();

    // --- LO pipeline buffers
    m_SphereVBO_LO            = GLBuffer(initLO * static_cast<uint32_t>(sizeof(CompactSphere)), nullptr, GL_DYNAMIC_DRAW);
    m_LOMem                   = std::make_unique<MemoryManager>(initLO);
    m_ChunkInfoSSBO_LO        = GLBuffer(m_MaxChunks * static_cast<uint32_t>(sizeof(ChunkInfoLO)), nullptr, GL_DYNAMIC_DRAW);
    m_DrawCmdSSBO_LO          = GLBuffer(m_MaxChunks * 4u * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
    m_VisibleCountBuf_LO      = GLBuffer(sizeof(uint32_t), &zero, GL_DYNAMIC_DRAW);
    m_VisibleChunkIdxSSBO_LO  = GLBuffer(m_MaxChunks * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
    m_ChunkInfoCPU_LO.reserve(m_MaxChunks);
    SetupVAO_LO();

    // --- Shaders
    m_CullerShader_HQ = std::make_unique<Shader>("res/shaders/frustum_culler.comp");
    m_CullerShader_LO = std::make_unique<Shader>("res/shaders/frustum_culler_lo.comp");
    m_AtomShader_HQ   = std::make_unique<Shader>("res/shaders/atom.vert",         "res/shaders/atom.frag");
    m_AtomShader_LO   = std::make_unique<Shader>("res/shaders/atom_compact.vert", "res/shaders/atom_compact.frag");

    m_AtomShader_HQ->Bind();
    m_AtomShader_HQ->SetFloat3("u_DirLight.direction", glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f)));
    m_AtomShader_HQ->SetFloat3("u_DirLight.ambient",   glm::vec3(0.5f));
    m_AtomShader_HQ->SetFloat3("u_DirLight.diffuse",   glm::vec3(0.5f));
    m_AtomShader_HQ->SetFloat3("u_DirLight.specular",  glm::vec3(0.5f));
    m_AtomShader_HQ->SetInt("u_Texture", 0);
    m_AtomShader_HQ->SetFloat("u_WorldScale", FEATURE_SCALE);
    m_AtomShader_HQ->Unbind();

    m_AtomShader_LO->Bind();
    m_AtomShader_LO->SetFloat3("u_DirLight.direction", glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f)));
    m_AtomShader_LO->SetFloat3("u_DirLight.ambient",   glm::vec3(0.5f));
    m_AtomShader_LO->SetFloat3("u_DirLight.diffuse",   glm::vec3(0.5f));
    m_AtomShader_LO->SetFloat3("u_DirLight.specular",  glm::vec3(0.5f));
    m_AtomShader_LO->SetFloat("u_WorldScale",   FEATURE_SCALE);
    m_AtomShader_LO->SetFloat("u_SphereRadius", SPHERE_RADIUS);
    m_AtomShader_LO->SetFloat("u_LodOversize", LOD_OVERSIZE);
    glUniform1ui(glGetUniformLocation(m_AtomShader_LO->GetRendererID(), "u_ChunkSize"), CHUNK_SIZE);
    m_AtomShader_LO->Unbind();

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    m_Initialized = true;
}

// --- VAO setup

void WorldRenderer::SetupVAO_HQ() {
    glCreateVertexArrays(1, &m_VAO_HQ);
    glVertexArrayVertexBuffer(m_VAO_HQ, 0, m_SphereVBO_HQ.GetRendererID(), 0,
                              static_cast<GLsizei>(sizeof(GPUSphere)));

    glEnableVertexArrayAttrib(m_VAO_HQ, 0);
    glVertexArrayAttribFormat(m_VAO_HQ, 0, 4, GL_FLOAT, GL_FALSE,
                              static_cast<GLuint>(offsetof(GPUSphere, PositionRadius)));
    glVertexArrayAttribBinding(m_VAO_HQ, 0, 0);

    glEnableVertexArrayAttrib(m_VAO_HQ, 1);
    glVertexArrayAttribIFormat(m_VAO_HQ, 1, 1, GL_UNSIGNED_SHORT,
                               static_cast<GLuint>(offsetof(GPUSphere, ChunkTypeAndFlags)));
    glVertexArrayAttribBinding(m_VAO_HQ, 1, 0);

    glEnableVertexArrayAttrib(m_VAO_HQ, 2);
    glVertexArrayAttribIFormat(m_VAO_HQ, 2, 1, GL_UNSIGNED_SHORT,
                               static_cast<GLuint>(offsetof(GPUSphere, AmbientOcclusion)));
    glVertexArrayAttribBinding(m_VAO_HQ, 2, 0);

    for (int i = 0; i < 6; ++i) {
        glEnableVertexArrayAttrib(m_VAO_HQ, 3 + i);
        glVertexArrayAttribFormat(m_VAO_HQ, 3 + i, 3, GL_UNSIGNED_BYTE, GL_TRUE,
                                  static_cast<GLuint>(offsetof(GPUSphere, Lights) + i * sizeof(LightVector)));
        glVertexArrayAttribBinding(m_VAO_HQ, 3 + i, 0);
    }
}

void WorldRenderer::SetupVAO_LO() {
    glCreateVertexArrays(1, &m_VAO_LO);
    glVertexArrayVertexBuffer(m_VAO_LO, 0, m_SphereVBO_LO.GetRendererID(), 0,
                              static_cast<GLsizei>(sizeof(CompactSphere)));

    // Location 0: ivec3 (lx, ly, lz) from 3 x int16
    glEnableVertexArrayAttrib(m_VAO_LO, 0);
    glVertexArrayAttribIFormat(m_VAO_LO, 0, 3, GL_SHORT,
                               static_cast<GLuint>(offsetof(CompactSphere, lx)));
    glVertexArrayAttribBinding(m_VAO_LO, 0, 0);

    // Location 1: uint (lodStep) from uint8
    glEnableVertexArrayAttrib(m_VAO_LO, 1);
    glVertexArrayAttribIFormat(m_VAO_LO, 1, 1, GL_UNSIGNED_BYTE,
                               static_cast<GLuint>(offsetof(CompactSphere, lodStep)));
    glVertexArrayAttribBinding(m_VAO_LO, 1, 0);
}

// --- SyncChunks

bool WorldRenderer::InSquare(ChunkCoordinates a, ChunkCoordinates b, int32_t dist) noexcept {
    return std::abs(a.X - b.X) <= dist && std::abs(a.Z - b.Z) <= dist;
}

void WorldRenderer::SyncChunks(WorldHandler& world) {
    if (!m_Initialized) return;

    const ChunkCoordinates newCenter   = world.GetCenterChunk();
    const ChunkCoordinates prevCenter  = m_LastSyncCenter;
    const bool             centerChanged = !(newCenter == m_LastSyncCenter);
    const bool             firstSync     = (m_LastSyncCenter.X == INT32_MAX);
    m_CamChunk       = newCenter;
    m_LastSyncCenter = newCenter;

    const int32_t renderD = static_cast<int32_t>(m_RenderDist);
    const int32_t hqLoadD = static_cast<int32_t>(m_HqLoadRange);

    if (centerChanged) {
        // --- LO eviction: trailing strip of render range
        auto tryEvictLO = [&](int32_t x, int32_t z) {
            const uint64_t key = ChunkCoordinates(x, z).GetKey();
            if (m_LOSlot.count(key)) EvictLO(key);
        };

        if (firstSync) {
            // Nothing to evict yet.
        } else if (std::abs(newCenter.X - prevCenter.X) > 2 * renderD
                || std::abs(newCenter.Z - prevCenter.Z) > 2 * renderD) {
            // No overlap with previous render ring - everything in m_LOSlot is gone.
            std::vector<uint64_t> toEvict; toEvict.reserve(m_LOSlot.size());
            for (const auto& [key, entry] : m_LOSlot) toEvict.push_back(key);
            for (uint64_t key : toEvict) EvictLO(key);
        } else {
            const int32_t newXMin = newCenter.X - renderD, newXMax = newCenter.X + renderD;
            const int32_t newZMin = newCenter.Z - renderD, newZMax = newCenter.Z + renderD;
            const int32_t prevXMin = prevCenter.X - renderD, prevXMax = prevCenter.X + renderD;
            const int32_t prevZMin = prevCenter.Z - renderD, prevZMax = prevCenter.Z + renderD;

            if (prevXMin < newXMin) {
                for (int32_t x = prevXMin; x < newXMin; ++x)
                    for (int32_t z = prevZMin; z <= prevZMax; ++z) tryEvictLO(x, z);
            }
            if (prevXMax > newXMax) {
                for (int32_t x = newXMax + 1; x <= prevXMax; ++x)
                    for (int32_t z = prevZMin; z <= prevZMax; ++z) tryEvictLO(x, z);
            }
            const int32_t overlapXMin = std::max(newXMin, prevXMin);
            const int32_t overlapXMax = std::min(newXMax, prevXMax);
            if (prevZMin < newZMin) {
                for (int32_t z = prevZMin; z < newZMin; ++z)
                    for (int32_t x = overlapXMin; x <= overlapXMax; ++x) tryEvictLO(x, z);
            }
            if (prevZMax > newZMax) {
                for (int32_t z = newZMax + 1; z <= prevZMax; ++z)
                    for (int32_t x = overlapXMin; x <= overlapXMax; ++x) tryEvictLO(x, z);
            }
        }

        // --- HQ eviction: HQ slot map is small (~hqLoadChunks); full scan is fine.
        {
            std::vector<uint64_t> toEvict;
            for (const auto& [key, entry] : m_HQSlot) {
                ChunkCoordinates cc; cc.SetFromKey(key);
                if (!InSquare(m_CamChunk, cc, hqLoadD)) toEvict.push_back(key);
            }
            for (uint64_t key : toEvict) EvictHQ(key);
        }

        // --- LO discovery: leading strip of render range. Catches chunks already
        // in the world cache that were outside the previous render ring.
        auto tryQueueLO = [&](int32_t x, int32_t z) {
            const ChunkCoordinates coords(x, z);
            const uint64_t key = coords.GetKey();
            if (m_LOSlot.count(key)) return;
            if (!world.GetChunk(coords)) return;
            const int32_t dx = x - m_CamChunk.X;
            const int32_t dz = z - m_CamChunk.Z;
            PendingUpload pu{};
            pu.key = key; pu.coords = coords;
            pu.distSq = static_cast<float>(dx * dx + dz * dz);
            m_PendingLO.push(pu);
        };

        const int32_t newXMin = newCenter.X - renderD, newXMax = newCenter.X + renderD;
        const int32_t newZMin = newCenter.Z - renderD, newZMax = newCenter.Z + renderD;

        if (firstSync
            || std::abs(newCenter.X - prevCenter.X) > 2 * renderD
            || std::abs(newCenter.Z - prevCenter.Z) > 2 * renderD) {
            for (int32_t z = newZMin; z <= newZMax; ++z)
                for (int32_t x = newXMin; x <= newXMax; ++x) tryQueueLO(x, z);
        } else {
            const int32_t prevXMin = prevCenter.X - renderD, prevXMax = prevCenter.X + renderD;
            const int32_t prevZMin = prevCenter.Z - renderD, prevZMax = prevCenter.Z + renderD;

            if (newXMin < prevXMin) {
                for (int32_t x = newXMin; x < prevXMin; ++x)
                    for (int32_t z = newZMin; z <= newZMax; ++z) tryQueueLO(x, z);
            }
            if (newXMax > prevXMax) {
                for (int32_t x = prevXMax + 1; x <= newXMax; ++x)
                    for (int32_t z = newZMin; z <= newZMax; ++z) tryQueueLO(x, z);
            }
            const int32_t overlapXMin = std::max(newXMin, prevXMin);
            const int32_t overlapXMax = std::min(newXMax, prevXMax);
            if (newZMin < prevZMin) {
                for (int32_t z = newZMin; z < prevZMin; ++z)
                    for (int32_t x = overlapXMin; x <= overlapXMax; ++x) tryQueueLO(x, z);
            }
            if (newZMax > prevZMax) {
                for (int32_t z = prevZMax + 1; z <= newZMax; ++z)
                    for (int32_t x = overlapXMin; x <= overlapXMax; ++x) tryQueueLO(x, z);
            }
        }

        // --- HQ promotion: full HQ_LOAD ring (small, 2401 chunks at defaults).
        for (int32_t dz = -hqLoadD; dz <= hqLoadD; ++dz) {
            for (int32_t dx = -hqLoadD; dx <= hqLoadD; ++dx) {
                const ChunkCoordinates coords(m_CamChunk.X + dx, m_CamChunk.Z + dz);
                const uint64_t key = coords.GetKey();
                if (m_HQSlot.count(key)) continue;
                if (!world.GetChunk(coords)) continue;
                PendingUpload pu{};
                pu.key    = key;
                pu.coords = coords;
                pu.distSq = static_cast<float>(dx * dx + dz * dz);
                m_PendingHQ.push(pu);
            }
        }
    }

    // Newly arrived chunks since last sync - delta produced by ChunkStreamer.
    std::vector<ChunkCoordinates> arrived = world.ConsumeRecentlyArrived();
    for (const ChunkCoordinates& coords : arrived) {
        const int32_t dx    = coords.X - m_CamChunk.X;
        const int32_t dz    = coords.Z - m_CamChunk.Z;
        const int32_t cheby = std::max(std::abs(dx), std::abs(dz));
        if (cheby > renderD) continue;

        const uint64_t key = coords.GetKey();
        const float distSq = static_cast<float>(dx * dx + dz * dz);

        if (!m_LOSlot.count(key)) {
            PendingUpload pu{};
            pu.key    = key;
            pu.coords = coords;
            pu.distSq = distSq;
            m_PendingLO.push(pu);
        }
        if (cheby <= hqLoadD && !m_HQSlot.count(key)) {
            PendingUpload pu{};
            pu.key    = key;
            pu.coords = coords;
            pu.distSq = distSq;
            m_PendingHQ.push(pu);
        }
    }
}

// --- UpdateUploads

void WorldRenderer::UpdateUploads(const WorldHandler& world) {
    if (!m_Initialized) return;

    const int32_t renderD = static_cast<int32_t>(m_RenderDist);
    const int32_t hqLoadD = static_cast<int32_t>(m_HqLoadRange);

    // HQ uploads first - this is what the player actually sees.
    {
        const uint32_t budget = std::min(HQ_UPLOAD_PER_FRAME,
                                         static_cast<uint32_t>(m_PendingHQ.size()));
        for (uint32_t i = 0; i < budget && !m_PendingHQ.empty(); ++i) {
            const PendingUpload pu = m_PendingHQ.top(); m_PendingHQ.pop();
            if (m_HQSlot.count(pu.key)) continue;
            if (!InSquare(m_CamChunk, pu.coords, hqLoadD)) continue;
            const Chunk* chunk = world.GetChunk(pu.coords);
            if (!chunk) continue;
            UploadHQ(*chunk);
        }
    }

    // LO uploads trickle in - small budget keeps the frame time stable while the
    // distant rings fill in over many frames.
    {
        const uint32_t budget = std::min(LO_UPLOAD_PER_FRAME,
                                         static_cast<uint32_t>(m_PendingLO.size()));
        for (uint32_t i = 0; i < budget && !m_PendingLO.empty(); ++i) {
            const PendingUpload pu = m_PendingLO.top(); m_PendingLO.pop();
            if (m_LOSlot.count(pu.key)) continue;
            if (!InSquare(m_CamChunk, pu.coords, renderD)) continue;
            const Chunk* chunk = world.GetChunk(pu.coords);
            if (!chunk) continue;
            UploadLO(*chunk);
        }
    }
}

// --- Upload helpers

bool WorldRenderer::UploadHQ(const Chunk& chunk) {
    std::vector<GPUSphere> spheres;
    chunk.GenerateMesh(spheres, SPHERE_RADIUS);
    if (spheres.empty()) return false;

    const uint32_t count = static_cast<uint32_t>(spheres.size());
    uint32_t offset = 0;
    bool allocated = m_HQMem->Allocate(count, offset);
    while (!allocated) {
        if (!GrowVBO_HQ()) {
            LOG_WARN("[WorldRenderer] HQ VBO full, dropping chunk (%d,%d)",
                     chunk.GetCoordinates().X, chunk.GetCoordinates().Z);
            return false;
        }
        allocated = m_HQMem->Allocate(count, offset);
    }

    m_SphereVBO_HQ.SetData(spheres.data(),
                           count * static_cast<uint32_t>(sizeof(GPUSphere)),
                           offset * static_cast<uint32_t>(sizeof(GPUSphere)));

    const BoundBox& bb  = chunk.GetBounds();
    const float     pad = SPHERE_RADIUS;
    ChunkInfo info{};
    info.offset = offset;
    info.size   = count;
    info.posX   = chunk.GetCoordinates().X;
    info.posZ   = chunk.GetCoordinates().Z;
    info.bbMinX = bb.m_Min.x - pad;  info.bbMinY = bb.m_Min.y - pad;  info.bbMinZ = bb.m_Min.z - pad;
    info.bbMaxX = bb.m_Max.x + pad;  info.bbMaxY = bb.m_Max.y + pad;  info.bbMaxZ = bb.m_Max.z + pad;

    HQEntry entry{};
    entry.slot      = static_cast<uint32_t>(m_ChunkInfoCPU_HQ.size());
    entry.vboOffset = offset;
    entry.vboSize   = count;

    m_ChunkInfoCPU_HQ.push_back(info);
    m_HQSlot[chunk.GetCoordinates().GetKey()] = entry;
    m_ChunkInfoHQDirty = true;
    return true;
}

bool WorldRenderer::UploadLO(const Chunk& chunk) {
    const ChunkLODSet& lods = chunk.GetLODs();
    if (lods.data.empty()) return false;

    const uint32_t totalCount = static_cast<uint32_t>(lods.data.size());
    uint32_t offset = 0;
    bool allocated = m_LOMem->Allocate(totalCount, offset);
    while (!allocated) {
        if (!GrowVBO_LO()) {
            LOG_WARN("[WorldRenderer] LO VBO full, dropping chunk (%d,%d)",
                     chunk.GetCoordinates().X, chunk.GetCoordinates().Z);
            return false;
        }
        allocated = m_LOMem->Allocate(totalCount, offset);
    }

    m_SphereVBO_LO.SetData(lods.data.data(),
                           totalCount * static_cast<uint32_t>(sizeof(CompactSphere)),
                           offset * static_cast<uint32_t>(sizeof(CompactSphere)));

    const BoundBox& bb = chunk.GetBounds();
    ChunkInfoLO info{};
    info.posX   = chunk.GetCoordinates().X;
    info.posZ   = chunk.GetCoordinates().Z;
    info.bbMinY = bb.m_Min.y;
    info.bbMaxY = bb.m_Max.y;
    for (uint32_t i = 0; i < LOD_LEVELS; ++i) {
        info.lodOffsets[i] = offset + lods.lodOffsets[i];
        info.lodCounts[i]  = lods.lodCounts[i];
    }

    LOEntry entry{};
    entry.slot         = static_cast<uint32_t>(m_ChunkInfoCPU_LO.size());
    entry.vboOffset    = offset;
    entry.vboTotalSize = totalCount;

    m_ChunkInfoCPU_LO.push_back(info);
    m_LOSlot[chunk.GetCoordinates().GetKey()] = entry;
    m_ChunkInfoLODirty = true;
    return true;
}

void WorldRenderer::EvictHQ(uint64_t key) {
    auto it = m_HQSlot.find(key);
    if (it == m_HQSlot.end()) return;
    const HQEntry entry = it->second;

    m_HQMem->Free(entry.vboOffset, entry.vboSize);

    const uint32_t last = static_cast<uint32_t>(m_ChunkInfoCPU_HQ.size()) - 1u;
    if (entry.slot != last) {
        const uint64_t lastKey =
            ChunkCoordinates(m_ChunkInfoCPU_HQ[last].posX, m_ChunkInfoCPU_HQ[last].posZ).GetKey();
        m_ChunkInfoCPU_HQ[entry.slot] = m_ChunkInfoCPU_HQ[last];
        m_HQSlot[lastKey].slot        = entry.slot;
    }
    m_ChunkInfoCPU_HQ.pop_back();
    m_HQSlot.erase(it);
    m_ChunkInfoHQDirty = true;
}

void WorldRenderer::EvictLO(uint64_t key) {
    auto it = m_LOSlot.find(key);
    if (it == m_LOSlot.end()) return;
    const LOEntry entry = it->second;

    m_LOMem->Free(entry.vboOffset, entry.vboTotalSize);

    const uint32_t last = static_cast<uint32_t>(m_ChunkInfoCPU_LO.size()) - 1u;
    if (entry.slot != last) {
        const uint64_t lastKey =
            ChunkCoordinates(m_ChunkInfoCPU_LO[last].posX, m_ChunkInfoCPU_LO[last].posZ).GetKey();
        m_ChunkInfoCPU_LO[entry.slot] = m_ChunkInfoCPU_LO[last];
        m_LOSlot[lastKey].slot        = entry.slot;
    }
    m_ChunkInfoCPU_LO.pop_back();
    m_LOSlot.erase(it);
    m_ChunkInfoLODirty = true;
}

// --- Render

void WorldRenderer::Render(const Camera& cam) {
    if (!m_Initialized) return;
    RenderLO(cam);
    RenderHQ(cam);
}

void WorldRenderer::RenderHQ(const Camera& cam) {
    if (m_ChunkInfoCPU_HQ.empty()) return;

    const uint32_t zero = 0;
    m_VisibleCountBuf_HQ.SetData(&zero, sizeof(uint32_t), 0);

    if (m_ChunkInfoHQDirty) {
        m_ChunkInfoSSBO_HQ.SetData(m_ChunkInfoCPU_HQ.data(),
            static_cast<uint32_t>(m_ChunkInfoCPU_HQ.size() * sizeof(ChunkInfo)), 0);
        m_ChunkInfoHQDirty = false;
    }

    Frustum frustum = cam.GetFrustum();
    frustum.Normalize();
    const float* planes = frustum.GetData();

    m_ChunkInfoSSBO_HQ.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
    m_DrawCmdSSBO_HQ.BindBase(GL_SHADER_STORAGE_BUFFER, 1);
    m_VisibleCountBuf_HQ.BindBase(GL_ATOMIC_COUNTER_BUFFER, 2);

    m_CullerShader_HQ->Bind();
    const GLuint cullID = m_CullerShader_HQ->GetRendererID();
    glUniform4fv(glGetUniformLocation(cullID, "u_FrustumPlanes"), 6, planes);
    glUniform1ui(glGetUniformLocation(cullID, "u_ChunkCount"),
                 static_cast<uint32_t>(m_ChunkInfoCPU_HQ.size()));
    glUniform2f(glGetUniformLocation(cullID, "u_CamChunkXZ"),
                static_cast<float>(m_CamChunk.X), static_cast<float>(m_CamChunk.Z));
    glUniform1ui(glGetUniformLocation(cullID, "u_HqRenderRange"), HQ_RENDER_RANGE);

    const uint32_t groups = (static_cast<uint32_t>(m_ChunkInfoCPU_HQ.size()) + 63u) / 64u;
    glDispatchCompute(groups, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
                    GL_COMMAND_BARRIER_BIT         |
                    GL_ATOMIC_COUNTER_BARRIER_BIT);

    const float focalLen = static_cast<float>(cam.GetHeight()) /
                           (2.0f * std::tan(cam.GetFovYRad() * 0.5f));
    float pointSizeRange[2] = {1.0f, 64.0f};
    glGetFloatv(GL_POINT_SIZE_RANGE, pointSizeRange);

    m_AtomShader_HQ->Bind();
    m_AtomShader_HQ->SetMat4("u_View",  cam.GetViewMatrix());
    m_AtomShader_HQ->SetMat4("u_Proj",  cam.GetProjMatrix());
    m_AtomShader_HQ->SetFloat3("u_CamPos",   cam.GetPosition());
    m_AtomShader_HQ->SetFloat3("u_CamUp",    cam.GetUp());
    m_AtomShader_HQ->SetFloat3("u_CamRight", cam.GetRight());
    m_AtomShader_HQ->SetFloat("u_FocalLength",        focalLen);
    m_AtomShader_HQ->SetFloat("u_OneOverFarDistance", 1.0f / cam.GetFar());
    m_AtomShader_HQ->SetFloat("u_MaxPointSize",       pointSizeRange[1]);

    const GLuint atomID = m_AtomShader_HQ->GetRendererID();
    glUniform2i(glGetUniformLocation(atomID, "u_Resolution"),
                cam.GetWidth(), cam.GetHeight());
    glUniform4fv(glGetUniformLocation(atomID, "u_FrustumPlanes"), 6, planes);

    m_DrawCmdSSBO_HQ.Bind(GL_DRAW_INDIRECT_BUFFER);
    m_VisibleCountBuf_HQ.Bind(GL_PARAMETER_BUFFER);
    glBindVertexArray(m_VAO_HQ);

    glMultiDrawArraysIndirectCount(GL_POINTS, nullptr, 0,
                                   static_cast<GLsizei>(m_ChunkInfoCPU_HQ.size()), 0);

    glBindVertexArray(0);
    m_DrawCmdSSBO_HQ.Unbind(GL_DRAW_INDIRECT_BUFFER);
    m_VisibleCountBuf_HQ.Unbind(GL_PARAMETER_BUFFER);
}

void WorldRenderer::RenderLO(const Camera& cam) {
    if (m_ChunkInfoCPU_LO.empty()) return;

    const uint32_t zero = 0;
    m_VisibleCountBuf_LO.SetData(&zero, sizeof(uint32_t), 0);

    if (m_ChunkInfoLODirty) {
        m_ChunkInfoSSBO_LO.SetData(m_ChunkInfoCPU_LO.data(),
            static_cast<uint32_t>(m_ChunkInfoCPU_LO.size() * sizeof(ChunkInfoLO)), 0);
        m_ChunkInfoLODirty = false;
    }

    Frustum frustum = cam.GetFrustum();
    frustum.Normalize();
    const float* planes = frustum.GetData();

    m_ChunkInfoSSBO_LO.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
    m_DrawCmdSSBO_LO.BindBase(GL_SHADER_STORAGE_BUFFER, 1);
    m_VisibleCountBuf_LO.BindBase(GL_ATOMIC_COUNTER_BUFFER, 2);
    m_VisibleChunkIdxSSBO_LO.BindBase(GL_SHADER_STORAGE_BUFFER, 3);

    m_CullerShader_LO->Bind();
    const GLuint cullID = m_CullerShader_LO->GetRendererID();
    // Largest LOD sphere radius drives the chunk-AABB padding so edge spheres
    // aren't false-culled. Largest block size = 1 << (LOD_LEVELS - 1).
    const float maxLodRadius = static_cast<float>(1u << (LOD_LEVELS - 1u))
                              * SPHERE_RADIUS * LOD_OVERSIZE;
    glUniform4fv(glGetUniformLocation(cullID, "u_FrustumPlanes"), 6, planes);
    glUniform1ui(glGetUniformLocation(cullID, "u_ChunkCount"),
                 static_cast<uint32_t>(m_ChunkInfoCPU_LO.size()));
    glUniform1f(glGetUniformLocation(cullID, "u_SphereRadius"),  SPHERE_RADIUS);
    glUniform1ui(glGetUniformLocation(cullID, "u_ChunkSize"),    CHUNK_SIZE);
    glUniform2f(glGetUniformLocation(cullID, "u_CamChunkXZ"),
                static_cast<float>(m_CamChunk.X), static_cast<float>(m_CamChunk.Z));
    glUniform1ui(glGetUniformLocation(cullID, "u_HqRenderRange"), HQ_RENDER_RANGE);
    glUniform1f(glGetUniformLocation(cullID, "u_RingFactor"),     LOD_RING_FACTOR);
    glUniform1f(glGetUniformLocation(cullID, "u_LodPad"),         maxLodRadius);

    const uint32_t groups = (static_cast<uint32_t>(m_ChunkInfoCPU_LO.size()) + 63u) / 64u;
    glDispatchCompute(groups, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT |
                    GL_COMMAND_BARRIER_BIT         |
                    GL_ATOMIC_COUNTER_BARRIER_BIT);

    const float focalLen = static_cast<float>(cam.GetHeight()) /
                           (2.0f * std::tan(cam.GetFovYRad() * 0.5f));
    float pointSizeRange[2] = {1.0f, 64.0f};
    glGetFloatv(GL_POINT_SIZE_RANGE, pointSizeRange);

    m_AtomShader_LO->Bind();
    m_AtomShader_LO->SetMat4("u_View",  cam.GetViewMatrix());
    m_AtomShader_LO->SetMat4("u_Proj",  cam.GetProjMatrix());
    m_AtomShader_LO->SetFloat3("u_CamPos",   cam.GetPosition());
    m_AtomShader_LO->SetFloat3("u_CamUp",    cam.GetUp());
    m_AtomShader_LO->SetFloat3("u_CamRight", cam.GetRight());
    m_AtomShader_LO->SetFloat("u_FocalLength",        focalLen);
    m_AtomShader_LO->SetFloat("u_OneOverFarDistance", 1.0f / cam.GetFar());
    m_AtomShader_LO->SetFloat("u_MaxPointSize",       pointSizeRange[1]);

    const GLuint atomID = m_AtomShader_LO->GetRendererID();
    glUniform2i(glGetUniformLocation(atomID, "u_Resolution"),
                cam.GetWidth(), cam.GetHeight());
    glUniform4fv(glGetUniformLocation(atomID, "u_FrustumPlanes"), 6, planes);

    m_DrawCmdSSBO_LO.Bind(GL_DRAW_INDIRECT_BUFFER);
    m_VisibleCountBuf_LO.Bind(GL_PARAMETER_BUFFER);
    glBindVertexArray(m_VAO_LO);

    glMultiDrawArraysIndirectCount(GL_POINTS, nullptr, 0,
                                   static_cast<GLsizei>(m_ChunkInfoCPU_LO.size()), 0);

    glBindVertexArray(0);
    m_DrawCmdSSBO_LO.Unbind(GL_DRAW_INDIRECT_BUFFER);
    m_VisibleCountBuf_LO.Unbind(GL_PARAMETER_BUFFER);
}

// --- GetSphereCount

uint32_t WorldRenderer::GetSphereCount() const noexcept {
    uint32_t total = 0;
    for (const auto& info : m_ChunkInfoCPU_HQ) total += info.size;
    for (const auto& info : m_ChunkInfoCPU_LO) {
        for (uint32_t i = 0; i < LOD_LEVELS; ++i) total += info.lodCounts[i];
    }
    return total;
}

// --- Shutdown

void WorldRenderer::Shutdown() {
    if (m_VAO_HQ) { glDeleteVertexArrays(1, &m_VAO_HQ); m_VAO_HQ = 0; }
    if (m_VAO_LO) { glDeleteVertexArrays(1, &m_VAO_LO); m_VAO_LO = 0; }

    m_CullerShader_HQ.reset();
    m_CullerShader_LO.reset();
    m_AtomShader_HQ.reset();
    m_AtomShader_LO.reset();

    m_ChunkInfoCPU_HQ.clear();
    m_ChunkInfoCPU_LO.clear();
    m_HQSlot.clear();
    m_LOSlot.clear();
    while (!m_PendingHQ.empty()) m_PendingHQ.pop();
    while (!m_PendingLO.empty()) m_PendingLO.pop();
    m_HQMem.reset();
    m_LOMem.reset();
    m_Initialized = false;
}

// --- VRAM estimation

static uint32_t QueryAvailableVRAMBytes() {
    GLint availableKB = 0;
    glGetIntegerv(0x9049 /* GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX */, &availableKB);
    if (glGetError() == GL_NO_ERROR && availableKB > 0)
        return static_cast<uint32_t>(std::min<uint64_t>(uint64_t(availableKB) * 1024ULL,
                                                        std::numeric_limits<uint32_t>::max()));
    GLint dedicatedKB = 0;
    glGetIntegerv(0x9047 /* GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX */, &dedicatedKB);
    if (glGetError() == GL_NO_ERROR && dedicatedKB > 0)
        return static_cast<uint32_t>(std::min<uint64_t>(uint64_t(dedicatedKB) * 1024ULL,
                                                        std::numeric_limits<uint32_t>::max()));
    return 0;
}

uint32_t WorldRenderer::EstimateMaxSpheresHQ() const {
    // HQ holds chunks across the full LOAD ring (HQ_LOAD_FACTOR * HQ_RENDER_RANGE).
    // Worst-case sphere count per chunk: surface + max gap fills =
    //   CHUNK_SIZE^2 * (1 + (CHUNK_SIZE - 1)/2) ~= 2048 for CHUNK_SIZE=16.
    constexpr uint32_t kPerChunk = 2048u;
    const uint32_t hqLoadChunks = (m_HqLoadRange * 2u + 1u) * (m_HqLoadRange * 2u + 1u);
    constexpr uint32_t kHardCap = 100'000'000u;
    const uint64_t budget = static_cast<uint64_t>(hqLoadChunks) * kPerChunk;
    return static_cast<uint32_t>(std::min<uint64_t>(budget, kHardCap));
}

uint32_t WorldRenderer::EstimateMaxSpheresLO() const {
    constexpr uint32_t kHardCap = 1'000'000'000u;
    const uint32_t vramBytes = QueryAvailableVRAMBytes();
    if (vramBytes == 0) return kHardCap;
    // Reserve 30% of VRAM for other resources and HQ pipeline.
    const uint64_t budget = static_cast<uint64_t>(vramBytes) * 7ULL / 10ULL;
    const uint32_t result = static_cast<uint32_t>(std::min<uint64_t>(budget / sizeof(CompactSphere),
                                                                     uint64_t(kHardCap)));
    LOG_INFO("[WorldRenderer] VRAM available: %u MB -> LO ceiling: %u spheres (~%u MB)",
             vramBytes / (1024u * 1024u), result,
             static_cast<uint32_t>(uint64_t(result) * sizeof(CompactSphere) / (1024u * 1024u)));
    return result;
}

// --- VBO growth

bool WorldRenderer::GrowVBO_HQ() {
    const uint32_t oldSpheres = m_SphereVBO_HQ.GetSize() / static_cast<uint32_t>(sizeof(GPUSphere));
    const uint32_t newSpheres = static_cast<uint32_t>(
        std::min<uint64_t>(uint64_t(oldSpheres) * 2u, uint64_t(m_MaxSpheresHQ)));
    if (newSpheres <= oldSpheres) return false;

    const uint32_t newBytes = newSpheres * static_cast<uint32_t>(sizeof(GPUSphere));
    while (glGetError() != GL_NO_ERROR) {}
    GLBuffer grown(newBytes, nullptr, GL_DYNAMIC_DRAW);
    if (glGetError() == GL_OUT_OF_MEMORY) {
        LOG_WARN("[WorldRenderer] GrowVBO_HQ: OOM at %u spheres", newSpheres);
        return false;
    }
    glCopyNamedBufferSubData(m_SphereVBO_HQ.GetRendererID(), grown.GetRendererID(),
                             0, 0, static_cast<GLsizeiptr>(m_SphereVBO_HQ.GetSize()));
    glVertexArrayVertexBuffer(m_VAO_HQ, 0, grown.GetRendererID(), 0,
                              static_cast<GLsizei>(sizeof(GPUSphere)));
    m_SphereVBO_HQ = std::move(grown);
    m_HQMem->Free(oldSpheres, newSpheres - oldSpheres);
    LOG_INFO("[WorldRenderer] HQ VBO grown: %u -> %u spheres", oldSpheres, newSpheres);
    return true;
}

bool WorldRenderer::GrowVBO_LO() {
    const uint32_t oldSpheres = m_SphereVBO_LO.GetSize() / static_cast<uint32_t>(sizeof(CompactSphere));
    const uint32_t newSpheres = static_cast<uint32_t>(
        std::min<uint64_t>(uint64_t(oldSpheres) * 2u, uint64_t(m_MaxSpheresLO)));
    if (newSpheres <= oldSpheres) return false;

    const uint32_t newBytes = newSpheres * static_cast<uint32_t>(sizeof(CompactSphere));
    while (glGetError() != GL_NO_ERROR) {}
    GLBuffer grown(newBytes, nullptr, GL_DYNAMIC_DRAW);
    if (glGetError() == GL_OUT_OF_MEMORY) {
        LOG_WARN("[WorldRenderer] GrowVBO_LO: OOM at %u spheres", newSpheres);
        return false;
    }
    glCopyNamedBufferSubData(m_SphereVBO_LO.GetRendererID(), grown.GetRendererID(),
                             0, 0, static_cast<GLsizeiptr>(m_SphereVBO_LO.GetSize()));
    glVertexArrayVertexBuffer(m_VAO_LO, 0, grown.GetRendererID(), 0,
                              static_cast<GLsizei>(sizeof(CompactSphere)));
    m_SphereVBO_LO = std::move(grown);
    m_LOMem->Free(oldSpheres, newSpheres - oldSpheres);
    LOG_INFO("[WorldRenderer] LO VBO grown: %u -> %u spheres", oldSpheres, newSpheres);
    return true;
}
