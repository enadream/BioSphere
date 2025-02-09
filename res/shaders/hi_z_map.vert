#version 460 core
layout(location = 0) in ivec3 a_SpherePos;  // Sphere position

uniform float u_Radius;

void main(){
    vec3 center = u_Radius * a_SpherePos;
    gl_Position = vec4(center, 1.0);
}