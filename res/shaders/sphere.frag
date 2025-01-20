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


out vec4 FragColor;
in vec3 v_FragPos;

// uniforms
uniform float u_FarDist;
uniform float u_ScaleFactor;

uniform vec3 u_Center;
uniform vec3 u_CameraPos;
uniform vec3 u_Color;

uniform Material u_Material;
uniform DirectLight u_DirLight;
uniform PointLight u_PointLight;

uniform samplerCube u_Texture;

// functions
vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 viewDir);

void main(){
    float dist2Center = length(v_FragPos - u_Center);
    float outerRadius = 0.50 * u_ScaleFactor;
    float innerRadius = 0.50;

    if (dist2Center > outerRadius) {
        discard;
    }

    
    vec3 centerToCam = u_CameraPos - u_Center;
    vec3 fragToCam = u_CameraPos - v_FragPos;
    // face normal
    //vec3 faceNormal = normalize(centerToCam);

    // calculate scaler value
    //float valX = clamp(dist2Center / innerRadius, 0.0, 1.0);
    //float scaler = sqrt(innerRadius*innerRadius - (dist2Center*dist2Center));

    
    float bVal = length(centerToCam);
    float dVal = length(fragToCam);
    float xSquare = dist2Center * dist2Center;
    float hVal = (bVal * dist2Center)/dVal;
    float hSquare = hVal * hVal;
    float rSquare = innerRadius * innerRadius;
    float yVal = sqrt(abs(rSquare - hSquare)) + sqrt(abs(xSquare - hSquare));


    gl_FragDepth = (dVal-yVal) / u_FarDist;

    vec3 viewDir = normalize(fragToCam);
    vec3 fragRealPos = yVal*viewDir + v_FragPos;
    vec3 fragNormal = normalize(fragRealPos - u_Center);

    vec3 resultColor = vec3(texture(u_Texture, fragNormal));
    //vec3 resultColor = CalcDirectLight(u_DirLight, fragNormal, viewDir);
    //vec3 resultColor = CalcPointLight(u_PointLight, fragNormal, viewDir);

    //vec3 fragRealPos = scaler*faceNormal + v_FragPos;
    //vec3 distVec = u_CameraPos - fragRealPos;
    // set depth value
    //gl_FragDepth = length(distVec)/u_FarDist;
   
    


    // smooth alpha blending near the edges
    //float alpha = 1.0 - smoothstep(innerRadius, outerRadius, dist2Center);

    // Set the final fragment color
    FragColor = vec4(resultColor, 1.0);

}

vec3 CalcDirectLight(DirectLight light, vec3 normal, vec3 viewDir){
    vec3 result;
    float weight;

    // ambient light
    vec3 textureColor = vec3(texture(u_Texture, normal));
    result = light.ambient * textureColor;

    // diffuse light
    weight = max(dot(-light.direction, normal), 0.0);
    result += weight * light.diffuse * textureColor;

    // specular light
    // vec3 reflectDir = reflect(light.direction, normal);
    // weight = pow(max(dot(reflectDir, viewDir), 0.0), 32.0f);
    // result += weight * light.specular * textureColor;

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