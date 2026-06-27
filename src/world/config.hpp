#pragma once

#include <cstdint>

constexpr uint8_t  CHUNK_SIZE              = 16;
constexpr float    SPHERE_RADIUS           = 0.5f;
constexpr int32_t  REGION_SIZE             = 64;    // chunks per region side (32x32)
constexpr uint32_t DEFAULT_RENDER_DISTANCE = 300;     // in chunk units
constexpr float    LOAD_DISTANCE_FACTOR    = 1.1f;  // load ring = renderDist x this

// One-knob terrain feature size.
// Scales feature WIDTH and HEIGHT together so their visual ratio stays the same.
// Smaller = more compact terrain, visible at shorter render distances.
//
//  1.0  = real-world scale  (mountains ~5 km wide, ~1 200 m tall) - needs render dist >> 1 600 m
//  0.1  = 10x compact       (mountains ~500 m wide, ~120 m tall)  - good for render dist 200
//  0.05 = 20x compact       (very dense, rocky landscape)
//
// The shader divides heights by this value to keep colour-band thresholds correct at any scale.
constexpr float FEATURE_SCALE = 1.0f;

// Radius (in chunks) of the full-detail rendering area centered on the player.
// Chunks inside this radius use full 40-byte GPUSphere data uploaded from RAM.
// Chunks outside use the compact 8-byte LOD format below.
constexpr uint32_t HQ_RENDER_RANGE = 32u;

// HQ chunks are uploaded for HQ_RENDER_RANGE * HQ_LOAD_FACTOR around the player
// but only rendered within HQ_RENDER_RANGE. The outer ring is a pre-loaded buffer
// so fast movement does not stall on per-frame HQ uploads.
constexpr float HQ_LOAD_FACTOR = 2.0f;

// Number of compact LOD levels stored per chunk in the low-quality VBO.
// Level i uses block size 2^i: LOD0=1, LOD1=2, LOD2=4, LOD3=8.
constexpr uint32_t LOD_LEVELS = 4u;

// Each LOD ring is this multiple of the previous ring's radius.
// Ring boundaries: HQ_RENDER_RANGE, *2, *4, *8 chunks.
constexpr float LOD_RING_FACTOR = 2.0f;

// Multiplier applied to compact LOD sphere radii. Adjacent LOD blocks at very
// different heights (mountain slopes) leave visible gaps unless the spheres are
// large enough to overlap. 1.0 = exact block size; 2.0 = covers slopes up to ~70 deg.
constexpr float LOD_OVERSIZE = 2.0f;

// Maximum chunks uploaded per frame, split per pipeline. HQ uploads are heavier
// (CPU mesh gen + larger byte payload) so they get a higher budget when needed;
// LO uploads are cheap but happen for many chunks at once - keep this small to
// avoid LO uploads stalling the visible HQ area.
constexpr uint32_t HQ_UPLOAD_PER_FRAME = 64u;
constexpr uint32_t LO_UPLOAD_PER_FRAME = 8u;

// Minimum XZ travel (in chunk units) before the load ring is recalculated.
// Prevents rapid load/unload when the player oscillates near a chunk boundary.
constexpr float CHUNK_UPDATE_DISTANCE = 8.0f;
