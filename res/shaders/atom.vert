#version 460 core
// This matches `glVertexAttribPointer`'s first argument (0) and `vec4` type.
layout (location = 0) in vec4 aPosition;
// ChunkTypeAndFlags (Layout Location 1)
layout (location = 1) in uint aChunkTypeFlags;
// AmbientOcclusion (Layout Location 2)
layout (location = 2) in uint aAmbientOcclusion;
// Note: We use `vec3` because the GL_UNSIGNED_BYTE data from the VBO
// is normalized to the range [0.0, 1.0] when you pass GL_TRUE to glVertexAttribPointer.
layout (location = 3) in vec3 aLights[6];

// layout (location = 0) in vec4 aSphere;
// layout (location = 1) in uvec2 aChunkTypeFlags;
// layout (location = 2) in uvec4 aLights;

out SphereData {
    vec3 center; // C
    float radius; // 
    vec3 centerToCam; // O - C
    float viewZOverFocalLength; // viewZ / focalLength 
    vec2 pixelCenter; // pixel center of the sphere
} v_Sphere;

out VertData {
    flat uint AmbientOcclusion;
} v_;

bool IsSphereVisible(vec3 pos, float radius);

// Uniforms 
uniform mat4 u_View;      // world-to-view matrix
uniform mat4 u_Proj;      // projection matrix
uniform vec3 u_CamPos;    // camera position
uniform vec3 u_CamUp;
uniform vec3 u_CamRight;
uniform ivec2 u_Resolution;

uniform vec4 u_FrustumPlanes[6];
uniform float u_MaxPointSize;  // maxium size of point
uniform float u_OneOverFarDistance; // (1.0 / Fardistance)
uniform float u_FocalLength;

void main(){
    vec3 sphereCenter = aPosition.xyz;
    float radius = aPosition.w;

    if (radius <= 0.0) {
        gl_Position = vec4(-1000.0, -1000.0, -1000.0, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    if (!IsSphereVisible(sphereCenter, radius)){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    
    // find the size ellipse size on the screen
    vec3 camToCenter = sphereCenter - u_CamPos;
    float oc = length(camToCenter);
    if (oc < radius * 1.01){ // if camera too close to the sphere don't render it
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    float oc2 = oc*oc;
    float scaledRadius = radius * oc / sqrt(oc2 - radius*radius);
    // find the center of the sphere based on camera view
    vec3 viewCenter = (u_View * vec4(sphereCenter, 1.0)).xyz;
    float zLenght = -viewCenter.z;
    // sMinor = (u_FocalLength * scaledRadius / -viewCenter.z)
    // sMinor = sMinor / cosAlpha
    float semiMajor = u_FocalLength * scaledRadius * oc / (zLenght * zLenght);
    float pointSize = semiMajor * 2.0;

    if (pointSize > u_MaxPointSize){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    // finding the center of the sphere in pixel coordinate
    vec4 projCenter = u_Proj * vec4(viewCenter, 1.0);
    vec3 ndc = projCenter.xyz / projCenter.w; // converting [-1, 1]

    // finding the ellipse center
    vec2 viewCenterXY = viewCenter.xy;
    float viewXYSize2 = dot(viewCenterXY, viewCenterXY);
    // H = sqrt(viewXYSize2)
    // offsetSize = (z^2 * H * OA^2) / (oc^4 - H^2*OA^2)
    float oa2 = scaledRadius*scaledRadius + oc*oc;
    float offsetSize = (zLenght*zLenght*oa2)/(oc2*oc2 - viewXYSize2*oa2);
    // because it will later be divided by H there is no need to multiply at the begining
    viewCenterXY *= offsetSize;

    // set out variables
    v_Sphere.center = sphereCenter;
    v_Sphere.radius = radius;
    v_Sphere.centerToCam = -camToCenter;
    v_Sphere.viewZOverFocalLength = zLenght / u_FocalLength;
    v_Sphere.pixelCenter = (ndc.xy*0.5 + 0.5) * u_Resolution;
    v_.AmbientOcclusion = aAmbientOcclusion;
    // set the values
    gl_Position = u_Proj * vec4(viewCenterXY, viewCenter.z, 1.0);;
    gl_PointSize = pointSize;
}



bool IsSphereVisible(vec3 pos, float radius){
    for (int i = 0; i < 6; i++){
        // Calculate signed distance from sphere center to the plane
        float dist = dot(pos, u_FrustumPlanes[i].xyz) + u_FrustumPlanes[i].w;

        // If the sphere is entirely behind the plane, it's not visible
        if (dist < -radius * 2.3) {
            return false;
        }
    }

    return true;
}