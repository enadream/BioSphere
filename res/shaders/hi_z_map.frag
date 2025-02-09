#version 460 core

// inputs
in vec3 v_FragPos;
in vec3 v_Center;
in float v_ScaledRadius;

// uniforms
uniform float u_FarDist;
uniform vec3 u_CameraPos;

void main() {
    // fragment distance to the circle center
    float dist2Center = length(v_FragPos - v_Center);

    if (dist2Center > v_ScaledRadius) {
        discard;
    }

    // calculate linear distance
    gl_FragDepth = distance(u_CameraPos, v_FragPos)*0.99999 / u_FarDist;
}
