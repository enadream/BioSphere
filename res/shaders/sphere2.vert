#version 460 core
layout(location = 0) in vec2 a_Pos;       // Base quad vertex
layout(location = 1) in uint a_SphereData;  // Sphere position

struct ChunkPos {
    int x;
    int z;
};

// outputs
out vec3 v_Center;
out vec3 v_FragPos;
out float v_DistCenterToCam;
out float v_ScaledRadius;

// uniforms
uniform mat4 u_ProjView;
uniform vec3 u_CameraPos;
uniform ChunkPos u_ChunkPos;

// this uniform can be discarded
uniform float u_FarDist;
uniform float u_Radius;

void main() {
    // Extract xOffset (uint8_t) from bits 24-31
    uint xOffset = a_SphereData & 0xFF;
    // Extract zOffset (uint8_t) from bits 16-23
    uint zOffset = (a_SphereData >> 8) & 0xFF;
    // Extract the lower 16 bits (yVal)
    uint yValRaw = (a_SphereData >> 16) & 0xFFFF;
    //Convert to a signed 16-bit integer:
    int yVal;
    if ((yValRaw & 0x8000) != 0) {
        // Negative value: Sign-extend to 32 bits
        yVal = int(yValRaw | 0xFFFF0000);
    } else {
        // Positive value
        yVal = int(yValRaw);
    }

    //Find the center of sphere
    v_Center = vec3((u_ChunkPos.x + xOffset) * u_Radius, yVal * u_Radius, (u_ChunkPos.z + zOffset) * u_Radius);

    vec3 centerToCam = u_CameraPos - v_Center;
    v_DistCenterToCam = length(centerToCam);

    // Early discard (better done via CPU culling first)
    if(v_DistCenterToCam < (u_Radius*1.001)) {
        gl_Position = vec4(-2.0, -2.0, -2.0, 1.0);
        return;
    }

    // calculate scaled radius value
    v_ScaledRadius  = (u_Radius * v_DistCenterToCam) / sqrt((v_DistCenterToCam - u_Radius)*(v_DistCenterToCam + u_Radius));

    // Billboard orientation
    vec3 viewDir = normalize(centerToCam);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), viewDir));
    vec3 up = cross(viewDir, right);

    // vertex position
    vec3 worldPos = v_Center + (right * a_Pos.x * v_ScaledRadius) + (up * a_Pos.y * v_ScaledRadius);

    v_FragPos = worldPos;
    gl_Position = u_ProjView * vec4(worldPos, 1.0);
}