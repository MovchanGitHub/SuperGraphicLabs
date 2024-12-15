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

const std::string model_path = "data/model.obj";
const std::string texture_path = "data/model.png";

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;


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
	glShaderSource(vShader, 1, &VertexShaderSource, NULL);
	// Компилируем шейдер
	glCompileShader(vShader);
	std::cout << "vertex shader \n";
	// Функция печати лога шейдера
	ShaderLog(vShader);
	// Создаем фрагментный шейдер
	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Передаем исходный код
	glShaderSource(fShader, 1, &FragShaderSource_UniformColorInShader, NULL);
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
	model = glm::rotate(model, angleY, glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
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
		Draw();
		window.display();
	}
	Release();
	return 0;
}