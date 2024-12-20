#version 330 core

#define FRAG_OUTPUT0 0

layout (location = FRAG_OUTPUT0) out vec4 color;

uniform struct PointLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec3 attenuation;
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

    float Ndot = max(dot(normal, lightDir), 0.0);
    if (Ndot > 0.9)
        color = material.diffuse * light.diffuse;
    else if (Ndot > 0.6)
        color = material.diffuse * light.diffuse * 0.6;
    else
        color = material.diffuse * light.diffuse * 0.1;

    color *= texture(material.texture, Vert.texcoord);
}