#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 v_FragPos;

uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_Model;

void main(){
    vec4 fragPos = u_Model * vec4(aPos, 1.0);
    gl_Position = u_Projection * u_View * fragPos;

    v_FragPos = fragPos.xyz;
}