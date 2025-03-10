#version 460 core

out vec4 FragColor;

// inputs
in SphereData {
    vec3 center;
    vec3 planeNormal;
    vec3 up;
    vec3 right;
    float radius;
} v_Sphere;

struct DirectLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform float u_FarDistance;
uniform vec3 u_CameraPos;
uniform vec3 u_CamUp;
uniform vec3 u_CamRight;
uniform DirectLight u_DirLight;

uniform samplerCube u_Texture;

vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir, vec3 fragRealPos);

void main() {
    // Translate gl_PointCoord from [0, 1] to [-0.5, 0.5]
    // The pivot point of gl_PointCoord is top left corner
    // Center it around (0,0): now range is [-0.5, 0.5]
    vec2 localCoord = gl_PointCoord - vec2(0.5, 0.5);
    //localCoord.y *= -1.0; // swap the y

    // Calculate the distance from the center
    float dist = 2.0 * length(localCoord);

    // If outside the circle's radius (0.5 in this case), discard the fragment.
    if (dist > 1.0) {
        discard;
    }

    // vec3 fragPosition = v_Sphere.center + (localCoord.x*v_Sphere.radius)*u_CamRight + (localCoord.y*v_Sphere.radius)*u_CamUp;
    // float xVal = dist * v_Sphere.radius; // dist in range [0,1] -> [0,r]
    // fragPosition += v_Sphere.planeNormal * sqrt((v_Sphere.radius + xVal)*(v_Sphere.radius - xVal));
    // vec3 fragNormal = normalize(fragPosition - v_Sphere.center);
    // gl_FragDepth = distance(u_CameraPos, fragPosition) / u_FarDistance;

    // vec3 viewDir = normalize(u_CameraPos - fragPosition);
    // vec3 resultColor = CalcDirectLight(u_DirLight, fragNormal, viewDir, fragPosition);
    // Otherwise, simply output a color for the circle.
    FragColor = vec4(dist,dist,dist, 1.0);
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
    vec3 textureColor = vec3(texture(u_Texture, normal));
    result = light.ambient * textureColor;

    // diffuse light
    weight = max(dot(-light.direction, normal), 0.0);
    result += weight * light.diffuse * textureColor;
    //result += pow(weight * light.diffuse * textureColor, vec3(2.2));

    // specular light phong
    //vec3 reflectDir = reflect(light.direction, normal);
    //weight = pow(max(dot(reflectDir, viewDir), 0.0), 32.0f);
    // specular light blinn phong
    vec3 halfWayDir = normalize(-light.direction + viewDir);
    weight = pow(max(dot(normal, halfWayDir), 0.0), 64.0f);
    result += weight * light.specular * textureColor;

    return result;
}