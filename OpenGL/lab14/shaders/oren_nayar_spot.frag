#version 330 core

in Vertex {
    vec2 texcoord;
    vec3 normal;
    vec3 lightDir;
    vec3 viewDir;
    float distance;
} Vert;

out vec4 color;

uniform float roughness;

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

void main(void)
{
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

    color = material.emission;
    color += material.ambient * light.ambient * attenuation;

    float roughness_squared = roughness * roughness;
    float a = 1.0f - 0.5f * (roughness_squared / (roughness_squared + 0.57f));
    float b = 0.45f * (roughness_squared / (roughness_squared + 0.09f));

    float angle_light_norm = acos(clamp(dot(normal, lightDir), 0.0f, 1.0f));
    float angle_view_norm = acos(clamp(dot(normal, viewDir), 0.0f, 1.0f));
    float alpha = max(angle_light_norm, angle_view_norm);
    float beta = min(angle_light_norm, angle_view_norm);
    float c = max(0.0f, cos(angle_light_norm - angle_view_norm)) * sin(alpha) * tan(beta);
    
    float diffuse_comp = max(0.0f, dot(normal, lightDir)) * (a + b * c);
    color += material.diffuse * light.diffuse * diffuse_comp * attenuation;

    color *= texture(material.texture, Vert.texcoord);
}