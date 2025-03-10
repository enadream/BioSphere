#version 460 core
layout (location = 0) in vec4 aSpherePos;

out SphereData {
    vec3 center; // C
    vec3 camToCenter; // L
    vec2 pixelCenter; // pixel center of the sphere
    float viewZOverFocalLength; // viewZ / focalLength 
    float radius; // r
    float pointSize; // gl point size
} v_Sphere;

// Uniforms supplied by the application:
uniform mat4 u_View;      // world-to-view matrix
uniform mat4 u_Proj;      // projection matrix
uniform vec3 u_CamPos;    // camera position
uniform ivec2 u_Resolution;

uniform float u_MaxDiameter;  // maxium size of point
uniform vec4 u_frustumPlanes[6];
uniform float u_FocalLength;

bool IsSphereVisible(vec3 pos, float radius);

void main(){
    vec3 sphereCenter = aSpherePos.xyz;
    float radius = aSpherePos.w;

    if (!IsSphereVisible(sphereCenter, radius)){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }
    
    // --- Cone and Plane Parameters ---
    vec3 camToCenter = sphereCenter - u_CamPos;
    float oc = length(camToCenter);

    if (oc < radius * 1.01){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    float oc2 = oc*oc;
    float scaledRadius = radius * oc / sqrt(oc2 - radius*radius);

    // find the center of the sphere based on camera view
    vec3 viewCenter = (u_View * vec4(sphereCenter, 1.0)).xyz;
    vec3 camViewDir = viewCenter / oc; // the length of the view center is oc

    // cos(alpha) = dot(vec3(0, 0, -1.0), camViewDir) = camViewDir.z * -1.0 = -camViewDir.z
    float cosAlpha = -camViewDir.z;
    // r at the focal point
    // semi-minor = b
    float b = u_FocalLength * scaledRadius / -viewCenter.z;
    // semi-major = a
    float a = b / cosAlpha;

    float pointSize = a * 2.0;

    if (pointSize > u_MaxDiameter){
        gl_Position = vec4(-1000,-1000,-1000, 1.0);
        gl_PointSize = 0.0;
        return;
    }


    // finding the center of the sphere in pixel coordinate
    vec4 pixelCenter = u_Proj * vec4(viewCenter, 1.0);
    vec3 ndc = pixelCenter.xyz / pixelCenter.w; // converting [-1, 1]

    
    // finding the ellipse center
    vec2 viewCenterXY = viewCenter.xy;
    float viewXYSize2 = dot(viewCenterXY, viewCenterXY);
    // H = sqrt(viewXYSize2)
    // offsetSize = (z^2 * H * OA^2) / (oc^4 - H^2*OA^2)
    float oa2 = scaledRadius*scaledRadius + oc*oc;
    float offsetSize = (viewCenter.z*viewCenter.z*oa2)/(oc2*oc2 - viewXYSize2*oa2);
    // because it will later be divided by H there is no need to multiply at the begining
    viewCenterXY *= offsetSize;
    vec3 ellipseCenterCamera = vec3(viewCenterXY, viewCenter.z);
    


    // float alpha = acos(-viewCenter.z / oc);
    // float beta = atan(scaledRadius / oc);
    // float centerSize = -viewCenter.z * ( tan(alpha+beta) + tan(alpha - beta)) / 2.0;

    // vec2 ellipseXY = viewCenter.xy;
    // float lenEllipseXY = dot(ellipseXY, ellipseXY);
    // ellipseXY = lenEllipseXY > 1e-8 ? ellipseXY * (centerSize / sqrt(lenEllipseXY)) : ellipseXY;  // Scale only if nonzero
    // vec3 ellipseCenterCamera = vec3(ellipseXY, viewCenter.z);


    // set out variables
    v_Sphere.center = sphereCenter;
    v_Sphere.camToCenter = -camToCenter;
    v_Sphere.pixelCenter = (ndc.xy*0.5 + 0.5) * u_Resolution;
    v_Sphere.viewZOverFocalLength = -viewCenter.z / u_FocalLength;
    v_Sphere.radius = radius;
    v_Sphere.pointSize = pointSize;

    // calculate the ellipse center on the screen
    // set the values
    gl_Position = u_Proj * vec4(ellipseCenterCamera, 1.0);
    gl_PointSize = pointSize;
}

bool IsSphereVisible(vec3 pos, float radius){
    for (int i = 0; i < 6; i++){
        // Calculate signed distance from sphere center to the plane
        float dist = dot(pos, u_frustumPlanes[i].xyz) + u_frustumPlanes[i].w;

        // If the sphere is entirely behind the plane, it's not visible
        if (dist < -radius * 2.3) {
            return false;
        }
    }

    return true;
}
