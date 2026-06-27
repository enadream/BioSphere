#version 460 core
out vec4 FragColor;

in SphereData {
    vec3 center; // C
    float radius; // 
    vec3 centerToCam; // O - C
    float viewZOverFocalLength; // viewZ / focalLength 
    vec2 pixelCenter; // pixel center of the sphere
} v_Sphere;

in VertData {
    flat uint AmbientOcclusion;
} v_;

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

uniform DirectLight u_DirLight;
uniform float u_WorldScale;

vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir, vec3 fragRealPos);
float GetAmbientOccVal(vec3 normal);

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
    float occ = GetAmbientOccVal(fragNormal);

    FragColor = vec4(fragColor * occ, 1.0);
}

// Deterministic per-sphere hash [0, 1].
// Uses only XZ (always exact grid multiples regardless of LOD level).
// Y is excluded: it is an averaged height that can vary slightly with FP rounding
// across compute dispatches, which would cause visible color changes on LOD rebuild.
float sphereHash(vec3 center) {
    vec2 p = round(center.xz * 2.0);
    p = fract(p * vec2(0.1031, 0.1030));
    p += dot(p, p.yx + 33.33);
    return fract((p.x + p.y) * p.x);
}

vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir, vec3 fragRealPos) {
    // Normalize by world scale so colour band thresholds stay at real-world metre values.
    float h = v_Sphere.center.y / u_WorldScale;

    // Height colour bands matched to the terrain scale (1 world unit = 1 m).
    vec3 deepOcean    = vec3(0.10, 0.28, 0.55);
    vec3 shallowOcean = vec3(0.22, 0.55, 0.82);
    vec3 beach        = vec3(0.76, 0.71, 0.50);
    vec3 lowland      = vec3(0.24, 0.50, 0.11);
    vec3 highland     = vec3(0.17, 0.36, 0.08);
    vec3 mountain     = vec3(0.44, 0.37, 0.28);
    vec3 highMountain = vec3(0.58, 0.56, 0.54);
    vec3 snow         = vec3(0.91, 0.93, 0.95);

    vec3 color;
    if      (h < -20.0)  color = deepOcean;
    else if (h <  -3.0)  color = mix(deepOcean,    shallowOcean, smoothstep( -20.0,   -3.0, h));
    else if (h <   5.0)  color = mix(shallowOcean, beach,        smoothstep(  -3.0,    5.0, h));
    // shorter green bands
    else if (h <  50.0)  color = mix(beach,        lowland,      smoothstep(   5.0,   50.0, h));
    else if (h < 180.0)  color = mix(lowland,      highland,     smoothstep(  50.0,  180.0, h));
    else if (h < 400.0)  color = mix(highland,     mountain,     smoothstep( 180.0,  400.0, h));
    else if (h < 800.0)  color = mix(mountain,     highMountain, smoothstep( 400.0,  800.0, h));
    else                 color = mix(highMountain, snow,         smoothstep( 800.0, 1400.0, h));

    // Per-sphere variation: unique random brightness/tint offset per sphere.
    float n = sphereHash(v_Sphere.center);
    color *= 0.88 + n * 0.24; // +-12 % brightness spread
    color  = clamp(color, 0.0, 1.0);

    // Ambient + diffuse lighting.
    vec3 result = light.ambient * color;
    float weight = max(dot(-light.direction, normal), 0.0);
    result += weight * light.diffuse * color;

    return result;
}

float GetAmbientOccVal(vec3 normal){
    const float shadePow = 1.0;

    const vec3 shadeDirs[12] = vec3[12](
        // XY plane: angles 45, 135, 225, 315 degrees
        shadePow * normalize(vec3(1.0, 1.0, 0.0)),
        shadePow * normalize(vec3(-1.0, 1.0, 0.0)),
        shadePow * normalize(vec3(-1.0, -1.0, 0.0)),
        shadePow * normalize(vec3(1.0, -1.0, 0.0)),
        // ZY plane: angles 45, 135, 225, 315 degrees
        shadePow * normalize(vec3(0.0, 1.0, 1.0)),
        shadePow * normalize(vec3(0.0, 1.0, -1.0)),
        shadePow * normalize(vec3(0.0, -1.0, -1.0)),
        shadePow * normalize(vec3(0.0, -1.0, 1.0)),
        // XZ plane: angles 45, 135, 225, 315 degrees
        shadePow * normalize(vec3(1.0, 0.0, 1.0)),
        shadePow * normalize(vec3(-1.0, 0.0, 1.0)),
        shadePow * normalize(vec3(-1.0, 0.0, -1.0)),
        shadePow * normalize(vec3(1.0, 0.0, -1.0))
    );

    float totalInfluence = 0.0;
    const float ambientExponent = 3.0;

    uint bitMask = 1;
    for (int i = 0; i < 12 ; i++){
        if ((bitMask & v_.AmbientOcclusion) != 0){
            float dotInfluence = max(0.0, dot(shadeDirs[i], normal));
            totalInfluence += pow(dotInfluence, ambientExponent);
        }
        bitMask = bitMask << 1;
    }

    if (totalInfluence > 0.99){
        totalInfluence = 0.99;
    }

    return 1.0 - totalInfluence;
}