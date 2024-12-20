#version 330 core

#define FRAG_OUTPUT0 0

layout (location = FRAG_OUTPUT0) out vec4 color;

uniform struct SpotLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec3 attenuation;
    vec3 spotDirection;
    float spotCosCutoff;
    float spotExponent;
} light;

uniform struct Material {
    sampler2D texture;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 emission;
    float shininess;
} material;

in Vertex {
    vec2 texcoord;
    vec3 normal;
    vec3 lightDir;
    vec3 viewDir;
    float distance;
} Vert;

void main() {
    vec3 normal = normalize(Vert.normal);
    vec3 lightDir = normalize(Vert.lightDir);
    vec3 viewDir = normalize(Vert.viewDir);

    // Направление света от прожектора
    vec3 spotDir = normalize(light.spotDirection);
    // Угол между направлением прожектора и направлением к точке
    float spotEffect = dot(spotDir, -lightDir);
    // Ограничение зоны влияния прожектора
    spotEffect = float(spotEffect > light.spotCosCutoff);
    // Экспоненциальное затухание
    spotEffect = max(pow(spotEffect, light.spotExponent), 0.0);

    // Коэффициент затухания прожектора
    float attenuation = spotEffect * (1.0 / max(light.attenuation[0] + 
        light.attenuation[1] * Vert.distance + 
        light.attenuation[2] * Vert.distance * Vert.distance, 0.0001));

    float Ndot = max(dot(normal, lightDir), 0.0);
    vec4 toonColor;
   
    if (Ndot > 0.9)
        toonColor = material.diffuse * light.diffuse;
    else if (Ndot > 0.6)
        toonColor = material.diffuse * light.diffuse * 0.6;
    else
        toonColor = material.diffuse * light.diffuse * 0.1;

    float RdotV = max(dot(reflect(-lightDir, normal), viewDir), 0.0);
    vec4 toonSpecular;

    if (RdotV > 0.9 && Ndot > 0.94)
        toonSpecular = material.specular * light.specular;
    else if (RdotV > 0.6 && Ndot > 0.94)
        toonSpecular = material.specular * light.specular * 0.3;
    else
        toonSpecular = vec4(0.0);

    color = material.emission;
    color += material.ambient * light.ambient * attenuation;
    color += toonColor * attenuation;
    color += toonSpecular;
    color *= texture(material.texture, Vert.texcoord);
}