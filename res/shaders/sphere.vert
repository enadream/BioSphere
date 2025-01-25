#version 460 core

layout (location = 0) in vec3 aCenterPosition;

void main(){
    gl_Position = vec4(aCenterPosition, 1.0);
}