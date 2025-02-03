#version 460 core
out vec4 FragColor;

void main(){
    FragColor = vec4(0.0, 1.0, 0.0, 0.1f);
    gl_FragDepth = 0.0;
}