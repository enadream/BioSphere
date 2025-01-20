#version 460 core

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

#define NR_POINT_LIGHTS 4

out vec4 FragColor;

in vec3 v_Normal;
in vec3 v_FragPos;
in vec2 v_TexCoords;

uniform Material u_Material;

uniform SpotLight u_SpotLight;
uniform DirectLight u_DirLight;
uniform PointLight u_PointLights[NR_POINT_LIGHTS];

uniform vec3 u_CameraPos;

// functions
vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 viewDir);

void main(){
    // properties
    vec3 norm = normalize(v_Normal);
    vec3 viewDir = normalize(u_CameraPos - v_FragPos);

    // phase 1: Directional lighting
    vec3 result = CalcDirectLight(u_DirLight, norm, viewDir);
    // phase 2: Point lights
    for (int i = 0; i < NR_POINT_LIGHTS; i++){
        result += CalcPointLight(u_PointLights[i], norm, viewDir);
    }
    // phase 3: Spot light
    result += CalcSpotLight(u_SpotLight, norm, viewDir);

    // emission lighting
    vec3 emission = vec3(texture(u_Material.EmissionTexture, v_TexCoords));
    //vec3(texture(u_Material.EmissionTexture, v_TexCoords));

    FragColor = vec4(result + emission, 1.0);
}

// light.direction and v_Normal has to be normalized
vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir){
    vec3 result;
    float weight;

    // ambient light
    result = light.ambient * vec3(texture(u_Material.DiffuseTexture, v_TexCoords));

    // diffuse light
    weight = max(dot(-light.direction, normal), 0.0);
    result += weight * light.diffuse * vec3(texture(u_Material.DiffuseTexture, v_TexCoords));

    // specular light
    vec3 reflectDir = reflect(light.direction, normal);
    weight = pow(max(dot(reflectDir, viewDir), 0.0), u_Material.Shininess);
    result += weight * light.specular * vec3(texture(u_Material.SpecularTexture, v_TexCoords));

    return result;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir){
    float weight;

    // ambient light
    vec3 ambient = light.ambient * vec3(texture(u_Material.DiffuseTexture, v_TexCoords));

    // diffuse light
    vec3 lightVec = light.position - v_FragPos;
    vec3 lightDir = normalize(lightVec);
    weight = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = weight * light.diffuse * vec3(texture(u_Material.DiffuseTexture, v_TexCoords));

    // specular light
    vec3 reflectDir = reflect(-lightDir, normal);
    weight = pow(max(dot(reflectDir, viewDir), 0.0), u_Material.Shininess);
    vec3 specular = weight * light.specular * vec3(texture(u_Material.SpecularTexture, v_TexCoords));

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
    vec3 ambient = light.ambient * vec3(texture(u_Material.DiffuseTexture, v_TexCoords));

    // diffusion light
    vec3 lightVec = light.position - v_FragPos;
    vec3 lightDir = normalize(lightVec);
    weight = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = weight * light.diffuse * vec3(texture(u_Material.DiffuseTexture, v_TexCoords));

    // specular light
    vec3 reflectDir = reflect(-lightDir, normal);
    weight = pow(max(dot(reflectDir, viewDir), 0.0), u_Material.Shininess);
    vec3 specular = weight * light.specular * vec3(texture(u_Material.SpecularTexture, v_TexCoords));

    // attenuation
    float distance = length(lightVec);
    float attenuation = 1.0 / (light.constant + light.linear*distance + light.quadratic*(distance*distance));

    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epislon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff)/epislon, 0.0, 1.0);

    // // combine results;
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return (ambient + diffuse + specular);
}