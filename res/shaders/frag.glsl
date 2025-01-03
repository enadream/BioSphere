#version 460 core
out vec4 FragColor;

//in vec3 ourColor;
in vec2 TexCoord;
in vec3 v_Normal;
in vec3 v_FragPos;

uniform float u_Color;
uniform sampler2D u_Texture;
uniform sampler2D u_Texture2;

uniform vec3 u_LightColor;
uniform vec3 u_LightPos;
uniform vec3 u_CameraPos;

void main(){
    // ambient lighting
    float ambientStrength = 0.2f;
    vec3 ambientColor = u_LightColor * ambientStrength;

    // diffuse lighting
    vec3 norm = normalize(v_Normal);
    vec3 lightDirection = normalize(u_LightPos - v_FragPos);
    float diffuse = max(dot(lightDirection, norm), 0.0f);

    // specular lighting
    float specularStrength = 0.5f;
    vec3 viewDir = normalize(u_CameraPos - v_FragPos);
    vec3 reflectDir = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    vec3 specular = specularStrength * spec * u_LightColor;

    vec4 result = mix(texture(u_Texture, TexCoord), texture(u_Texture2, TexCoord), 0.2);
    result.xyz *= (ambientColor + diffuse + specular);

    FragColor = result;
}