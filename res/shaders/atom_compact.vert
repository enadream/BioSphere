#version 460 core

// Compact 8-byte vertex stream
//   location 0: ivec3 chunk-local position in half-radius units (int16 x 3)
//   location 1: uint  block-size step (uint8) -> world radius = step * SPHERE_RADIUS
layout (location = 0) in ivec3 aLocal;
layout (location = 1) in uint  aLodStep;

out SphereData {
    vec3 center;
    float radius;
    vec3 centerToCam;
    float viewZOverFocalLength;
    vec2 pixelCenter;
} v_Sphere;

struct ChunkInfoLO {
    int   posX;
    int   posZ;
    float bbMinY;
    float bbMaxY;
    uint  lodOffsets[4];
    uint  lodCounts[4];
};

layout(std430, binding = 0) readonly buffer ChunkInfoLOBuffer {
    ChunkInfoLO chunks[];
};

// Written by frustum_culler_lo.comp: maps gl_DrawID -> chunk slot index.
layout(std430, binding = 3) readonly buffer VisibleChunkIdxBuffer {
    uint visibleChunkIdx[];
};

uniform mat4  u_View;
uniform mat4  u_Proj;
uniform vec3  u_CamPos;
uniform vec3  u_CamUp;
uniform vec3  u_CamRight;
uniform ivec2 u_Resolution;

uniform vec4  u_FrustumPlanes[6];
uniform float u_MaxPointSize;
uniform float u_OneOverFarDistance;
uniform float u_FocalLength;

uniform float u_SphereRadius;
uniform uint  u_ChunkSize;
uniform float u_LodOversize;

bool IsSphereVisible(vec3 pos, float radius);

void main(){
    uint chunkIdx = visibleChunkIdx[gl_DrawID];
    ChunkInfoLO ci = chunks[chunkIdx];

    float halfR  = u_SphereRadius * 0.5;
    float originX = float(ci.posX) * float(u_ChunkSize) * u_SphereRadius;
    float originZ = float(ci.posZ) * float(u_ChunkSize) * u_SphereRadius;

    vec3 sphereCenter = vec3(
        originX + float(aLocal.x) * halfR,
        float(aLocal.y) * halfR,
        originZ + float(aLocal.z) * halfR
    );
    float radius = float(aLodStep) * u_SphereRadius * u_LodOversize;

    if (radius <= 0.0) {
        gl_Position = vec4(-1000.0, -1000.0, -1000.0, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    if (!IsSphereVisible(sphereCenter, radius)) {
        gl_Position = vec4(-1000.0, -1000.0, -1000.0, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    vec3  camToCenter = sphereCenter - u_CamPos;
    float oc          = length(camToCenter);
    if (oc < radius * 1.01) {
        gl_Position = vec4(-1000.0, -1000.0, -1000.0, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    float oc2          = oc * oc;
    float scaledRadius = radius * oc / sqrt(oc2 - radius * radius);
    vec3  viewCenter   = (u_View * vec4(sphereCenter, 1.0)).xyz;
    float zLength      = -viewCenter.z;
    float semiMajor    = u_FocalLength * scaledRadius * oc / (zLength * zLength);
    float pointSize    = semiMajor * 2.0;

    if (pointSize > u_MaxPointSize) {
        gl_Position = vec4(-1000.0, -1000.0, -1000.0, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    vec4 projCenter = u_Proj * vec4(viewCenter, 1.0);
    vec3 ndc        = projCenter.xyz / projCenter.w;

    vec2  viewCenterXY = viewCenter.xy;
    float viewXYSize2  = dot(viewCenterXY, viewCenterXY);
    float oa2          = scaledRadius * scaledRadius + oc * oc;
    float offsetSize   = (zLength * zLength * oa2) / (oc2 * oc2 - viewXYSize2 * oa2);
    viewCenterXY *= offsetSize;

    v_Sphere.center               = sphereCenter;
    v_Sphere.radius               = radius;
    v_Sphere.centerToCam          = -camToCenter;
    v_Sphere.viewZOverFocalLength = zLength / u_FocalLength;
    v_Sphere.pixelCenter          = (ndc.xy * 0.5 + 0.5) * u_Resolution;
    gl_Position                   = u_Proj * vec4(viewCenterXY, viewCenter.z, 1.0);
    gl_PointSize                  = pointSize;
}

bool IsSphereVisible(vec3 pos, float radius) {
    for (int i = 0; i < 6; i++) {
        float dist = dot(pos, u_FrustumPlanes[i].xyz) + u_FrustumPlanes[i].w;
        if (dist < -radius * 2.3) return false;
    }
    return true;
}
