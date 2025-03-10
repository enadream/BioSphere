#version 460 core
layout (location = 0) in vec4 aSpherePos;

struct Sphere {
    vec4 position;
};

// outputs
out SphereData {
    vec3 center;
    vec3 planeNormal;
    vec3 up;
    vec3 right;
    float radius;
} v_Sphere;


// uniforms
uniform mat4 u_ProjView;
uniform mat4 u_View;
uniform vec3 u_CameraPos;
uniform vec3 u_CameraFront;
uniform vec3 u_CamUp;
uniform ivec2 u_ScreenSize;
uniform float u_FocalLength;
uniform float u_MaxDiameter;


// Declare an SSBO (binding point 0) for large spheres:
layout(std430, binding = 0) buffer LargeSpheres {
    uint count; // atomic counter at the start of the buffer
    Sphere data[];
};

void main(){
    v_Sphere.center = aSpherePos.xyz;
    v_Sphere.radius = aSpherePos.w;

    // Calculate the clip-space position.
    vec4 clipPosCenter = u_ProjView * vec4(v_Sphere.center, 1.0);
    gl_Position = clipPosCenter;

    // // Transform the position to view space.
    // vec4 viewPos = u_View * vec4(v_Sphere.center, 1.0);

    // float depth = -viewPos.z;  // depth > 0 if in front of the camera.
    // if (depth < v_Sphere.radius){
    //     gl_Position = vec4(-1000,-1000,-1000, 1.0);
    //     gl_PointSize = 0.0;
    //     return;
    // }

    // // d: distance from camera to sphere center
    // // Compute D and d
    vec3 centerToCam = u_CameraPos - v_Sphere.center;
    v_Sphere.planeNormal = normalize(centerToCam);
    // float dist = length(D);
    // float sphereScreenRadius = (v_Sphere.radius / sqrt((dist + v_Sphere.radius)*(dist - v_Sphere.radius))) * u_FocalLength;
    // sphereScreenRadius *= dist / dot(u_CameraFront, D);
    // calculate scaled radius value
    float dist = length(centerToCam);
    float scaledRadius = (v_Sphere.radius*dist) / sqrt((dist - v_Sphere.radius)*(dist + v_Sphere.radius));
    // determine the right and up vectors
    v_Sphere.right = normalize(cross(vec3(0.0, 1.0, 0.0), v_Sphere.planeNormal)); // right vector perpendicular to viewDir
    v_Sphere.up = normalize(cross(v_Sphere.planeNormal, v_Sphere.right)); // recompute an accurate up vector
    vec3 upVector = scaledRadius * v_Sphere.right + v_Sphere.center;
    vec4 clipPosUp = u_ProjView * vec4(upVector, 1.0);

    // 2. Perform perspective division to obtain normalized device coordinates (NDC).
    vec2 ndcA = clipPosCenter.xy / clipPosCenter.w;
    vec2 ndcB = clipPosUp.xy / clipPosUp.w;

    // 3. This is done by mapping NDC, which is in [-1,1], to [0, viewportSize]
    vec2 screenA = ((ndcA + 1.0) * 0.5) * u_ScreenSize;
    vec2 screenB = ((ndcB + 1.0) * 0.5) * u_ScreenSize;

    // 4. Compute the Euclidean distance between the two screen-space positions.
    float pixelSize = length(screenB - screenA);

    // // finding screen radius
    // float screenRadius = scaledRadius * u_FocalLength / depth;
    //float diameter = sphereScreenRadius * 2.0;
    float diameter = 2.0 * pixelSize; 

    if (diameter > u_MaxDiameter){
        // Atomically append this sphere's data into the SSBO.
        uint index = atomicAdd(count, 1u);
        //data[index].position = aSpherePos;

        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    } else {
        // Set the point size so that the point sprite (a square) scales correctly.
        gl_PointSize = diameter;
    }

    

    
}



