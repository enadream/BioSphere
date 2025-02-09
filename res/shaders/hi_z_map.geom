#version 460 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

// outputs
out vec3 v_Center;
out vec3 v_FragPos;
out float v_ScaledRadius;

// uniforms
uniform mat4 u_ProjView;
uniform vec3 u_CameraPos;
uniform float u_Radius;

void main () {
    v_Center = gl_in[0].gl_Position.xyz;
    
    float v_DistCenterToCam = length(u_CameraPos - v_Center);

    if (v_DistCenterToCam < (u_Radius*1.001)){
        return;
    }

    // calculate scaled radius value
    v_ScaledRadius = (u_Radius*v_DistCenterToCam) / sqrt((v_DistCenterToCam - u_Radius)*(v_DistCenterToCam + u_Radius));

    // top left, top right, bottom left, bottom right
    vec2 offsetSc[4] = {vec2(-1.0, 1.0), vec2(1.0, 1.0), vec2(-1.0,-1.0), vec2(1.0, -1.0)};
    vec3 viewDir = normalize(v_Center - u_CameraPos);

    // determine the right and up vectors
    vec3 up = vec3(0.0, 1.0, 0.0); // world up
    vec3 right = normalize(cross(up, viewDir)); // right vector perpendicular to viewDir
    up = normalize(cross(viewDir, right)); // recompute an accurate up vector

    for (int i = 0; i < 4; i++){
        // v_FragPos is position in global space
        v_FragPos = v_ScaledRadius*offsetSc[i].x*right + v_ScaledRadius*offsetSc[i].y*up + v_Center;
        gl_Position = u_ProjView * vec4(v_FragPos, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}