#version 460 core
//layout (early_fragment_tests) in; // Force early testing
//layout (depth_less) out float gl_FragDepth; // Hint for driver

struct Material {
    sampler2D DiffuseTexture;
    sampler2D SpecularTexture;
    sampler2D EmissionTexture;
    float Shininess;
};
struct DirectLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
struct PointLight {
    vec3 position;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
	
    float constant;
    float linear;
    float quadratic;
};

out vec4 FragColor;

// inputs
in vec3 v_Center;
in float v_DistCenterToCam;
in vec3 v_FragPos;
in float v_ScaledRadius;
in float g_Radius;

// uniforms
uniform float u_FarDist;
uniform vec3 u_CameraPos;
uniform vec3 u_Color;

uniform Material u_Material;
uniform DirectLight u_DirLight;
uniform PointLight u_PointLight;

uniform samplerCube u_Texture;

// functions
vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir, vec3 fragRealPos);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 viewDir);

void main(){
    float dist2Center = length(v_FragPos - v_Center);

    if (dist2Center > v_ScaledRadius) {
        discard;
    }

    vec3 fragToCam = u_CameraPos - v_FragPos;

    //float bVal = v_DistCenterToCam
    float dVal = length(fragToCam);
    float xSquare = dist2Center * dist2Center;
    float hVal = (v_DistCenterToCam * dist2Center)/dVal;
    float hSquare = hVal * hVal;
    float rSquare = g_Radius * g_Radius;
    float yVal = sqrt(abs(rSquare - hSquare)) + sqrt(abs(xSquare - hSquare));

    gl_FragDepth = (dVal-yVal) / u_FarDist;

    vec3 viewDir = normalize(fragToCam);
    vec3 fragRealPos = yVal*viewDir + v_FragPos;
    vec3 fragNormal = normalize(fragRealPos - v_Center);

    vec3 resultColor = CalcDirectLight(u_DirLight, fragNormal, viewDir, fragRealPos);

    FragColor = vec4(1.0,1.0,1.0, 1.0);
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
    vec3 textureColor = color;//vec3(texture(u_Texture, normal));
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

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir){
    float weight;

    // ambient light
    vec3 ambient = light.ambient * u_Color;

    // diffuse light
    vec3 lightVec = light.position - v_FragPos;
    vec3 lightDir = normalize(lightVec);
    weight = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = weight * light.diffuse * u_Color;

    // specular light
    vec3 reflectDir = reflect(-lightDir, normal);
    weight = pow(max(dot(reflectDir, viewDir), 0.0), 32.0f);
    vec3 specular = weight * light.specular * u_Color;

    // attenuation
    float distance = length(lightVec);
    float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*(distance*distance));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 viewDir){
    float weight;
    
    // ambient light
    vec3 ambient = light.ambient * u_Color;

    // diffusion light
    vec3 lightVec = light.position - v_FragPos;
    vec3 lightDir = normalize(lightVec);
    weight = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = weight * light.diffuse * u_Color;

    // specular light
    vec3 reflectDir = reflect(-lightDir, normal);
    weight = pow(max(dot(reflectDir, viewDir), 0.0), 32.0f);
    vec3 specular = weight * light.specular * u_Color;

    // attenuation
    float distance = length(lightVec);
    //float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*(distance*distance));

    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epislon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff)/epislon, 0.0, 1.0);

    // // combine results;
    ambient *= intensity;
    diffuse *= intensity;
    specular *= intensity;

    return (ambient + diffuse + specular);
}