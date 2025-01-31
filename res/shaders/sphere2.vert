#version 460 core;

layout(location = 0) in vec2 a_Pos;       // Base quad vertex
layout(location = 1) in float a_Radius;
layout(location = 2) in vec3 a_InstancePos;

// outputs
out vec3 v_Center;
out vec3 v_FragPos;
out float v_DistCenterToCam;
out float v_ScaledRadius;

// uniforms
uniform mat4 u_ViewProj;
uniform vec3 u_CameraPos;
uniform vec3 u_WorldUp;

// this uniform can be discarded
uniform float u_FarDist;

void main() {
    vec3 centerToCam = u_CameraPos - a_InstancePos;

    v_Center = a_InstancePos;
    v_DistCenterToCam = length(centerToCam);

    // Early discard (better done via CPU culling first)
    if(v_DistCenterToCam > u_FarDist || v_DistCenterToCam < (a_Radius*1.001)) {
        gl_Position = vec4(0);
        return;
    }

    // calculate scaled radius value
    v_ScaledRadius = (a_Radius * v_DistCenterToCam) / sqrt((v_DistCenterToCam - a_Radius)*(v_DistCenterToCam + a_Radius));

    // Billboard orientation
    vec3 viewDir = normalize(centerToCam);
    vec3 right = normalize(cross(u_WorldUp, viewDir));
    vec3 up = cross(viewDir, right);

    // vertex position
    vec3 worldPos = a_InstancePos + (right * a_Pos.x * v_ScaledRadius) + (up * a_Pos.y * v_ScaledRadius);

    v_FragPos = worldPos;
    gl_Position = u_ViewProj * vec4(worldPos, 1.0);
}