#version 330 core

#define FRAG_OUTPUT0 0

layout (location = FRAG_OUTPUT0) out vec4 color;

uniform struct DirLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
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
} Vert;

void main() {
    vec3 normal = normalize(Vert.normal);
    vec3 lightDir = normalize(Vert.lightDir);
    vec3 viewDir = normalize(Vert.viewDir);

    color = material.emission;
    color += material.ambient * light.ambient;

    float Ndot = max(dot(normal, lightDir), 0.0);
    color += material.diffuse * light.diffuse * Ndot;

    float RdotVpow = max(pow(dot(reflect(-lightDir, normal), viewDir), material.shininess), 0.0);
    color += material.specular * light.specular * RdotVpow;

    color *= texture(material.texture, Vert.texcoord);
}