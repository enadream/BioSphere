#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform isampler2D u_Tex;

void main() {
    int rawValue = texture(u_Tex, TexCoords).r;
    // Convert the integer bits back to a float.
    float value = intBitsToFloat(rawValue);

    FragColor = vec4(value, value, value, 1.0);
}