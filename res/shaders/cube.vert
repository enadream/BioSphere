#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 v_Normal;
out vec3 v_FragPos;
out vec2 v_TexCoords;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main(){
    vec4 fragPos = u_Model * vec4(aPos, 1.0f);
    gl_Position = u_Projection * u_View * fragPos;
    v_FragPos = fragPos.xyz;
    v_Normal = mat3(transpose(inverse(u_Model))) * aNormal;
    v_TexCoords = aTexCoords;
}