#version 460 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

// outputs
out vec3 v_Center;
out vec3 v_FragPos;
out float v_DistCenterToCam;
out float v_ScaledRadius;
//out vec3 v_Color;

// uniforms
uniform mat4 u_Projection;
uniform mat4 u_View;

// camera informations
uniform vec3 u_CameraPos;

uniform float u_Radius;
uniform float u_FarDist;

void main(){
    vec3 center = gl_in[0].gl_Position.xyz;
    
    float distCenterToCam = length(u_CameraPos - center);

    if (distCenterToCam > u_FarDist || distCenterToCam < (u_Radius*1.001)){
        return;
    }
    
    // calculate scaled radius value
    float scaledRadius = (u_Radius*distCenterToCam) / sqrt((distCenterToCam - u_Radius)*(distCenterToCam + u_Radius));

    // top left, top right, bottom left, bottom right
    vec2 offsetSc[4] = {vec2(-1.0, 1.0), vec2(1.0, 1.0), vec2(-1.0,-1.0), vec2(1.0, -1.0)};

    vec3 viewDir = normalize(center - u_CameraPos);

    // determine the right and up vectors
    vec3 up = vec3(0.0, 1.0, 0.0); // world up
    vec3 right = normalize(cross(up, viewDir)); // right vector perpendicular to viewDir
    up = normalize(cross(viewDir, right)); // recompute an accurate up vector
    
    v_Center = center;
    v_DistCenterToCam = distCenterToCam;
    v_ScaledRadius = scaledRadius;
    
    for (int i = 0; i < 4; i++){
        vec3 vertex = scaledRadius*offsetSc[i].x*right + scaledRadius*offsetSc[i].y*up + center;
        v_FragPos = vertex;
        gl_Position = u_Projection * u_View * vec4(vertex, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}

