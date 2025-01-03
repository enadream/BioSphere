#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTextCoord;
layout(location = 2) in vec3 aNormal;

out vec2 TexCoord;
out vec3 v_Normal;
out vec3 v_FragPos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat3 u_Normal;

void main(){
    vec4 fragPos = u_Model * vec4(aPos, 1.0f);

    TexCoord = aTextCoord;
    v_FragPos = fragPos.xyz;  
    v_Normal = u_Normal * aNormal;

    gl_Position = u_Projection * u_View * fragPos;
}