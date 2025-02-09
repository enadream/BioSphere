#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_Tex;

void main() {
    float depth = texture(u_Tex, TexCoords).r;
    FragColor = vec4(depth, depth, depth, 1.0);
    gl_FragDepth = 0.0;
}