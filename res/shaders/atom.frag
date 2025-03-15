#version 460 core
out vec4 FragColor;

in SphereData {
    vec3 center; // C
    float radius; // 
    vec3 centerToCam; // O - C
    float viewZOverFocalLength; // viewZ / focalLength 
    vec2 pixelCenter; // pixel center of the sphere
} v_Sphere;

struct DirectLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

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

uniform samplerCube u_Texture;
uniform DirectLight u_DirLight;

vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir, vec3 fragRealPos);

void main() {
    // Calculate the pixel offset from the defined center
    vec2 pixelOffset = gl_FragCoord.xy - v_Sphere.pixelCenter;
    // (pixelOffset / u_FocalLength) * v_Sphere.viewZ;
    vec2 worldOffset = pixelOffset * v_Sphere.viewZOverFocalLength;
    vec3 camToFrag = (v_Sphere.center + worldOffset.x * u_CamRight + worldOffset.y * u_CamUp) - u_CamPos;

    // calculate discriminant
    float a = dot(camToFrag, camToFrag);
    float b = 2.0 * dot(camToFrag, v_Sphere.centerToCam);
    float c = dot(v_Sphere.centerToCam, v_Sphere.centerToCam) - v_Sphere.radius*v_Sphere.radius;
    float discriminant = b*b - 4.0*a*c;

    if (discriminant < 0.0){
        //FragColor = vec4(1.0,0.0,0.0, 1.0);
        discard;
    }
    float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
    vec3 fragPos = u_CamPos + t1 * camToFrag;
    vec3 fragNormal = (fragPos - v_Sphere.center) / v_Sphere.radius;

    gl_FragDepth = distance(fragPos, u_CamPos) * u_OneOverFarDistance;
    vec3 fragColor = CalcDirectLight(u_DirLight, fragNormal, -normalize(camToFrag), fragPos);

    FragColor = vec4(fragColor, 1.0);
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
    // vec3 reflectDir = reflect(light.direction, normal);
    // weight = pow(max(dot(reflectDir, viewDir), 0.0), 32.0f);
    // // // specular light blinn phong
    // // vec3 halfWayDir = normalize(-light.direction + viewDir);
    // // weight = pow(max(dot(normal, halfWayDir), 0.0), 16.0f);
    // result += weight * light.specular * textureColor;

    return result;
}
