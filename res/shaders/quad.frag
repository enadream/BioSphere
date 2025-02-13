#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_Tex;

void main() {
    vec4 rawValue = texture(u_Tex, TexCoords);
    // // Convert the integer bits back to a float.
    // float value = intBitsToFloat(rawValue);

    FragColor = rawValue;
}