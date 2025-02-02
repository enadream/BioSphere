#version 460 core

layout(location = 0) in vec2 a_Pos;       // Base quad vertex
layout(location = 1) in vec3 a_InstancePos;

// outputs
out vec3 v_Center;
out vec3 v_FragPos;
out float v_DistCenterToCam;
out float v_ScaledRadius;

// uniforms
uniform mat4 u_ProjView;
uniform vec3 u_CameraPos;

// this uniform can be discarded
uniform float u_FarDist;
uniform float u_Radius;

void main() {
    vec3 centerToCam = u_CameraPos - a_InstancePos;

    v_Center = a_InstancePos;
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
    vec3 worldPos = a_InstancePos + (right * a_Pos.x * v_ScaledRadius) + (up * a_Pos.y * v_ScaledRadius);

    v_FragPos = worldPos;
    gl_Position = u_ProjView * vec4(worldPos, 1.0);
}