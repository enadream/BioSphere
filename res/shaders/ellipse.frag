#version 460 core

out vec4 FragColor;

// varying
in SphereData {
    vec3 center; // C
    vec3 camToCenter; // L
    vec2 pixelCenter; // pixel center of the sphere
    float viewZOverFocalLength; // viewZ / focalLength 
    float radius; // r
} v_Sphere;

struct DirectLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// uniforms camera informations
uniform vec3 u_CamPos;
uniform vec3 u_CamUp;
uniform vec3 u_CamRight;
uniform vec3 u_CamForw;
uniform mat4 u_InvProj;
uniform mat4 u_InvViewMatrix;
uniform mat3 u_CamRotation;
uniform ivec2 u_Resolution;
//uniform vec2 u_Scale;  // vec2(tan(uFOV/2)*aspect, tan(uFOV/2)) computed on CPU
uniform vec2 u_TanHalfFOV; // x is the horizontal half fov, y is the vertical fov

uniform float u_FocalLength;
uniform float u_FarDistance;
uniform samplerCube u_Texture;
uniform DirectLight u_DirLight;

vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir, vec3 fragRealPos);

void main(){
    //vec2 ndc = (gl_FragCoord.xy / u_Resolution) * 2.0 - 1.0;
    //vec3 viewDir = normalize(u_CamForw + ndc.x*u_CamRight*u_TanHalfFOV.x + ndc.y*u_CamUp*u_TanHalfFOV.y);

    // Calculate the pixel offset from the defined center
    vec2 pixelOffset = gl_FragCoord.xy - v_Sphere.pixelCenter;

    // Convert pixel offset into a world offset using the focal length.
    // (Divide by focal length and then multiply by depth.)
    // (pixelOffset / u_FocalLength) * v_Sphere.viewZ;
    vec2 worldOffset = pixelOffset * v_Sphere.viewZOverFocalLength;

    // Compute the world position on the plane.
    // Since uPlaneCenter = uCameraPos + d * cameraForward,
    // the world point is the center plus the offset along right and up.
    //vec3 worldPos = v_Sphere.center + worldOffset.x * u_CamRight + worldOffset.y * u_CamUp;
    //vec3 viewDir = normalize((v_Sphere.center + worldOffset.x * u_CamRight + worldOffset.y * u_CamUp) - u_CamPos);

    // // Ray-Sphere Intersection - Geometric Solution 
    // float tCA = dot(v_Sphere.camToCenter, viewDir);
    // // d^2 = L^2 - tCA^2
    // float dSquare = dot(v_Sphere.camToCenter, v_Sphere.camToCenter) - tCA*tCA;
    // float rSquare = v_Sphere.radius*v_Sphere.radius;
    // if (dSquare > rSquare){
    //     //FragColor = vec4(1.0,0.0,0.0, 1.0);
    //     // d cannot be bigger than radius, if it's there is no interception
    //     discard;
    // }

    // float tHC = sqrt(rSquare - dSquare);
    // float t1 = tCA - tHC; // intersection point 1
    //// float t2 = tCA + tHC; // intersection point 2

    vec3 viewDir = (v_Sphere.center + worldOffset.x * u_CamRight + worldOffset.y * u_CamUp) - u_CamPos;

    float a = dot(viewDir, viewDir);
    float b = 2.0 * dot(viewDir, v_Sphere.camToCenter);
    float c = dot(v_Sphere.camToCenter, v_Sphere.camToCenter) - v_Sphere.radius*v_Sphere.radius;
    float discriminant = b*b - 4.0*a*c;

    if (discriminant < 0.0){
        //FragColor = vec4(1.0,0.0,0.0, 1.0);
        discard;
    }
    float t1 = (-b - sqrt(discriminant)) / (2.0 * a);


    vec3 fragPos = u_CamPos + t1 * viewDir;
    vec3 fragNormal = (fragPos - v_Sphere.center) / v_Sphere.radius;

    // if (dot(fragNormal, normalize(v_Sphere.camToCenter)) > 0.9999){
    //     FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    //     return;
    // }

    //viewDir = -normalize(viewDir);
    //vec3 fragColor = CalcDirectLight(u_DirLight, fragNormal, viewDir, fragPos);
    gl_FragDepth = distance(fragPos, u_CamPos) / u_FarDistance;

    FragColor = vec4(fragNormal, 1.0);
}


vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir, vec3 fragRealPos){
    vec3 result;
    float weight;

    vec3 color;

    if (fragRealPos.y < 1.0){
        color = vec3(0.0, 0.0, 1.0);
    }
    else if (fragRealPos.y < 50.0){
        float val = (fragRealPos.y-1.0) / 50.0; // 1-50 range
        color = mix(vec3(0.7411764, 0.396078, 0.0), vec3(1.0, 0.729, 0.42), val);
    } else {
        float val = (fragRealPos.y-50.0) / 50.0; // 50-100 range
        color = mix(vec3(1.0, 0.729, 0.42), vec3(1.0, 1.0, 1.0), val); ;
    }

    // ambient light
    vec3 textureColor = color; //vec3(texture(u_Texture, normal));
    result = light.ambient * textureColor;

    // diffuse light
    weight = max(dot(-light.direction, normal), 0.0);
    result += weight * light.diffuse * textureColor;
    //result += pow(weight * light.diffuse * textureColor, vec3(2.2));

    // specular light phong
    vec3 reflectDir = reflect(light.direction, normal);
    weight = pow(max(dot(reflectDir, viewDir), 0.0), 32.0f);
    // // specular light blinn phong
    // vec3 halfWayDir = normalize(-light.direction + viewDir);
    // weight = pow(max(dot(normal, halfWayDir), 0.0), 16.0f);
    result += weight * light.specular * textureColor;

    return result;
}
