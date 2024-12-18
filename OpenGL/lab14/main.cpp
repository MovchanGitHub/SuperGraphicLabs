#include <cmath>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <random>
#include "model.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

Model model0;
GLuint instanceVBO;

const std::string model_path = "data/bus2.obj";
const std::string texture_path = "data/bus2.png";

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;

// точечный источник света
glm::vec4 lightPos(3.0f, 3.0f, 3.0f, 1.0f);
glm::vec4 lightAmbient(0.1f, 0.1f, 0.1f, 1.0f);
glm::vec4 lightDiffuse(0.8f, 0.8f, 0.8f, 1.0f);
glm::vec4 lightSpecular(1.0f, 1.0f, 1.0f, 1.0f);
//glm::vec3 attenuation(1.0f, 0.09f, 0.032f);
glm::vec3 attenuation(1.0f, 0.0f, 0.0f);

// Исходный код вершинного шейдера
const char* VertexShaderSource = R"(
 #version 330 core
 layout (location = 0) in vec3 position;
 layout (location = 1) in vec3 normal;
 layout (location = 2) in vec2 tex_coord;
 layout (location = 3) in mat4 orbit_transform;
 // следующий будет location = 7

 out vec2 out_tex_coord; 

 uniform mat4 model;
 uniform mat4 view;
 uniform mat4 projection;

 void main() {
	out_tex_coord = vec2(tex_coord.x, 1.0f - tex_coord.y);
	gl_Position = projection * view * model * orbit_transform * vec4(position, 1.0);
 }
)";

// Исходный код фрагментного шейдера из третьего задания
const char* FragShaderSource_UniformColorInShader = R"(
 #version 330 core
 in vec2 out_tex_coord;

 out vec4 color;

 uniform sampler2D tex;
 
 void main() {
	color = texture(tex, out_tex_coord);
 }
)";

const char* PhongVertexShader = R"(
#version 330 core

#define VERT_POSITION 0
#define VERT_NORMAL 1
#define VERT_TEXCOORD 2

layout (location = VERT_POSITION) in vec3 position;
layout (location = VERT_NORMAL) in vec3 normal;
layout (location = VERT_TEXCOORD) in vec2 texcoord;

uniform struct Transform {
    mat4 model;
    mat4 viewProjection;
    mat3 normal;
    vec3 viewPosition;
} transform;

uniform struct PointLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec3 attenuation;
} light;

out Vertex {
    vec2 texcoord;
    vec3 normal;
    vec3 lightDir;
    vec3 viewDir;
    float distance;
} Vert;

void main() {
    vec4 vertex = transform.model * vec4(position, 1.0);
    vec4 lightDir = light.position - vertex;
    gl_Position = transform.viewProjection * vertex;
    Vert.texcoord = vec2(texcoord.x, 1.0f - texcoord.y);
    Vert.normal = transform.normal * normal;
    Vert.lightDir = normalize(vec3(lightDir));
    Vert.viewDir = normalize(transform.viewPosition - vec3(vertex));
    Vert.distance = length(lightDir);
}
)";

const char* PhongFragShader = R"(
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
    vec3 viewDir = normalize(Vert.viewDir);

    float attenuation = 1.0 / max(light.attenuation[0] + 
        light.attenuation[1] * Vert.distance + 
        light.attenuation[2] * Vert.distance * Vert.distance, 0.0001);

    color = material.emission;
    color += material.ambient * light.ambient * attenuation;

    float Ndot = max(dot(normal, lightDir), 0.0);
    color += material.diffuse * light.diffuse * Ndot * attenuation;

    float RdotVpow = max(pow(dot(reflect(-lightDir, normal), viewDir), material.shininess), 0.0);
    color += material.specular * light.specular * RdotVpow * attenuation;

    color *= texture(material.texture, Vert.texcoord);
}
)";


// ID шейдерной программы
GLuint Program;

void ShaderLog(unsigned int shader) {
	int infologLen = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 1)
	{
		int charsWritten = 0;
		std::vector<char> infoLog(infologLen);
		glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog.data());
		std::cout << "InfoLog: " << infoLog.data() << std::endl;
	}
}

void InitShader() {
	// Создаем вершинный шейдер
	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	// Передаем исходный код
	glShaderSource(vShader, 1, &PhongVertexShader, NULL);
	// Компилируем шейдер
	glCompileShader(vShader);
	std::cout << "vertex shader \n";
	// Функция печати лога шейдера
	ShaderLog(vShader);
	// Создаем фрагментный шейдер
	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Передаем исходный код
	glShaderSource(fShader, 1, &PhongFragShader, NULL);
	// Компилируем шейдер
	glCompileShader(fShader);
	std::cout << "fragment shader \n";
	// Функция печати лога шейдера
	ShaderLog(fShader);
	// Создаем программу и прикрепляем шейдеры к ней
	Program = glCreateProgram();
	glAttachShader(Program, vShader);
	glAttachShader(Program, fShader);
	// Линкуем шейдерную программу
	glLinkProgram(Program);
	// Проверяем статус сборки
	int link_ok;
	glGetProgramiv(Program, GL_LINK_STATUS, &link_ok);
	if (!link_ok) {
		std::cout << "error attach shaders \n";
		return;
	}
}



void Init() {
	// Шейдеры
	InitShader();
	InitTranslations();
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.5, 0.5, 0.5, 0.0);
	model0 = Model(model_path, texture_path);
	glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 0.0f));
	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	glUseProgram(Program);
	glUniformMatrix4fv(glGetUniformLocation(Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUseProgram(0);
}

float angleX = 0.0f;
float angleY = 0.0f;

float aspectRatio;

void Draw() {
	glUseProgram(Program); // Устанавливаем шейдерную программу текущей
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);

	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glUniformMatrix4fv(glGetUniformLocation(Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

	glm::mat4 model = glm::rotate(glm::mat4(1.0f), angleX, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
	//model = glm::rotate(model, angleY, glm::vec3(0.0f, 1.0f, 0.0f));

	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.viewProjection"), 1, GL_FALSE, glm::value_ptr(projection * view));
	glUniformMatrix3fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	glUniform3fv(glGetUniformLocation(Program, "transform.viewPosition"), 1, glm::value_ptr(cameraPos));

	//glUniformMatrix4fv(glGetUniformLocation(Program, "model"), 1, GL_FALSE, glm::value_ptr(model));

	glUniform4fv(glGetUniformLocation(Program, "light.position"), 1, glm::value_ptr(lightPos));
	glUniform4fv(glGetUniformLocation(Program, "light.ambient"), 1, glm::value_ptr(lightAmbient));
	glUniform4fv(glGetUniformLocation(Program, "light.diffuse"), 1, glm::value_ptr(lightDiffuse));
	glUniform4fv(glGetUniformLocation(Program, "light.specular"), 1, glm::value_ptr(lightSpecular));
	glUniform3fv(glGetUniformLocation(Program, "light.attenuation"), 1, glm::value_ptr(attenuation));

	glUniform1i(glGetUniformLocation(Program, "material.texture"), 0); // Текстура привязана к текстурному блоку 0
	glUniform4f(glGetUniformLocation(Program, "material.ambient"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(glGetUniformLocation(Program, "material.diffuse"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(glGetUniformLocation(Program, "material.specular"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(glGetUniformLocation(Program, "material.emission"), 0.0f, 0.0f, 0.0f, 1.0f);
	glUniform1f(glGetUniformLocation(Program, "material.shininess"), 32.0f);

	model0.display_model(Program);
	glUseProgram(0); // Отключаем шейдерную программу
}

// Освобождение шейдеров
void ReleaseShader() {
	// Передавая ноль, мы отключаем шейдерную программу
	glUseProgram(0);
	// Удаляем шейдерную программу
	glDeleteProgram(Program);
}

void Release() {
	// Шейдеры
	ReleaseShader();
	// Вершинный буфер
	model0.release();
}

void HandleKeyboardInput() {
	constexpr float cameraSpeed = 0.05f; // Скорость перемещения
	constexpr float rotationSpeed = 0.2f;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) cameraPos += cameraSpeed * cameraFront;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) cameraPos -= cameraSpeed * cameraFront;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) pitch += rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) pitch -= rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) yaw -= rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) yaw += rotationSpeed;

	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;
}

int main() {
	sf::Window window(sf::VideoMode(600, 600), "My OpenGL window", sf::Style::Default, sf::ContextSettings(24));
	window.setVerticalSyncEnabled(true);
	window.setActive(true);
	glewInit();
	Init();
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) { window.close(); }
			else if (event.type == sf::Event::Resized) { glViewport(0, 0, event.size.width, event.size.height); }
		}
		HandleKeyboardInput();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		sf::Vector2u windowSize = window.getSize();
		aspectRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
		Draw();
		window.display();
	}
	Release();
	return 0;
}