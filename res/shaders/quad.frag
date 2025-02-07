#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_Tex;

void main() {
    //vec3 texCol = vec3(TexCoords.x, TexCoords.y, 0.0);
    vec3 texCol = texture(u_Tex, TexCoords).rgb;
    FragColor = vec4(texCol, 1.0);
    gl_FragDepth = 0.0;
}