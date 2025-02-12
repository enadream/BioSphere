#version 460 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

// inputs
in float v_Radius[];

// outputs
out vec3 v_Center;
out float v_DistCenterToCam;
out vec3 v_FragPos;
out float v_ScaledRadius;
out float g_Radius;

// uniforms
uniform mat4 u_ProjView;
uniform vec3 u_CameraPos;
uniform vec4 u_frustumPlanes[6];

// functions
bool IsSphereVisible(vec3 pos, float radius);

void main(){
    vec3 center = gl_in[0].gl_Position.xyz;
    float radius = v_Radius[0];

    // frustum culling
    if (!IsSphereVisible(center, radius)){
        return;
    }

    float distCenterToCam = length(u_CameraPos - center);
    if (distCenterToCam < (radius*1.001)){
        return;
    }
    
    // calculate scaled radius value
    float scaledRadius = (radius*distCenterToCam) / sqrt((distCenterToCam - radius)*(distCenterToCam + radius));

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
    g_Radius = radius;
    
    for (int i = 0; i < 4; i++){
        vec3 vertex = scaledRadius*offsetSc[i].x*right + scaledRadius*offsetSc[i].y*up + center;
        v_FragPos = vertex;
        gl_Position = u_ProjView * vec4(vertex, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}

bool IsSphereVisible(vec3 pos, float radius){
    for (int i = 0; i < 6; i++){
        // Calculate signed distance from sphere center to the plane
        float dist = dot(pos, u_frustumPlanes[i].xyz) + u_frustumPlanes[i].w;

        // If the sphere is entirely behind the plane, it's not visible
        if (dist < -radius) {
            return false;
        }
    }

    return true;
}