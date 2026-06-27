#version 460 core
out vec4 FragColor;

in SphereData {
    vec3 center;
    float radius;
    vec3 centerToCam;
    float viewZOverFocalLength;
    vec2 pixelCenter;
} v_Sphere;

struct DirectLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform mat4  u_View;
uniform mat4  u_Proj;
uniform vec3  u_CamPos;
uniform vec3  u_CamUp;
uniform vec3  u_CamRight;
uniform ivec2 u_Resolution;

uniform vec4  u_FrustumPlanes[6];
uniform float u_MaxPointSize;
uniform float u_OneOverFarDistance;
uniform float u_FocalLength;

uniform DirectLight u_DirLight;
uniform float       u_WorldScale;

float sphereHash(vec3 center) {
    vec2 p = round(center.xz * 2.0);
    p = fract(p * vec2(0.1031, 0.1030));
    p += dot(p, p.yx + 33.33);
    return fract((p.x + p.y) * p.x);
}

vec3 BiomeColor(float worldY) {
    float h = worldY / u_WorldScale;

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

    return color;
}

void main() {
    vec2 pixelOffset = gl_FragCoord.xy - v_Sphere.pixelCenter;
    vec2 worldOffset = pixelOffset * v_Sphere.viewZOverFocalLength;
    vec3 camToFrag   = (v_Sphere.center + worldOffset.x * u_CamRight + worldOffset.y * u_CamUp) - u_CamPos;

    float a            = dot(camToFrag, camToFrag);
    float b            = 2.0 * dot(camToFrag, v_Sphere.centerToCam);
    float c            = dot(v_Sphere.centerToCam, v_Sphere.centerToCam) - v_Sphere.radius * v_Sphere.radius;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0) discard;

    float t1         = (-b - sqrt(discriminant)) / (2.0 * a);
    vec3  fragPos    = u_CamPos + t1 * camToFrag;
    vec3  fragNormal = (fragPos - v_Sphere.center) / v_Sphere.radius;

    gl_FragDepth = distance(fragPos, u_CamPos) * u_OneOverFarDistance;

    // Color from the actual hit position, not the sphere center. Oversized LOD
    // spheres heavily overlap on slopes; using center y would let the "winning"
    // sphere flicker as the camera moves, since each sphere carries a different
    // band y. Fragment y stays stable for the same surface point.
    vec3  color  = BiomeColor(fragPos.y);
    float n      = sphereHash(v_Sphere.center);
    color *= 0.88 + n * 0.24;
    color  = clamp(color, 0.0, 1.0);

    vec3  result = u_DirLight.ambient * color;
    float weight = max(dot(-u_DirLight.direction, fragNormal), 0.0);
    result += weight * u_DirLight.diffuse * color;

    FragColor = vec4(result, 1.0);
}
