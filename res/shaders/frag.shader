#version 460 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform float u_Color;
uniform sampler2D u_Texture;

void main(){
    // vec4(ourColor * u_Color, 1.0f);
    FragColor = texture(u_Texture, TexCoord) * vec4(ourColor * u_Color, 1.0f);
}