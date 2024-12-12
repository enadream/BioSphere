#version 460 core

in vec3 ourColor;
uniform float u_Color;

out vec4 FragColor;

void main(){
    FragColor = vec4(ourColor * u_Color, 1.0f);
}