#version 460 core
layout (location = 0) in ivec3 aSpherePos;
layout (location = 1) in float radius;

out float v_Radius;

void main(){
    vec3 pos = radius * aSpherePos;
    v_Radius = radius;
    
    gl_Position = vec4(pos, 1.0);
}