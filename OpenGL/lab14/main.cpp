#include <cmath>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <random>
#include "model.h"
#include "shader.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

Model model0;
Model model1;
Model model2;
Model tree_model;
Model ground_model;
GLuint instanceVBO;

const std::string model_path = "data/Cearadactylus.obj";
const std::string texture_path = "data/Cearadactylus.jpg";
const std::string model_path1 = "data/Elasmosaurus.obj";
const std::string texture_path1 = "data/Elasmosaurus.jpg";
const std::string model_path2 = "data/Triceratops.obj";
const std::string texture_path2 = "data/Triceratops.png";
const std::string model_path3 = "data/treeBirch.obj";
const std::string texture_path3 = "data/treeBirch.jpg";
const std::string model_path4 = "data/cube.obj";
const std::string texture_path4 = "data/ground.jpg";

enum class light_kind {PointLightSource, Spotlight, DirLightSource};
constexpr light_kind LIGHT_KIND = light_kind::PointLightSource;

enum class shader_kind {Phong, OrenNayar, Toon, ToonSpecular};
constexpr shader_kind SHADER_KIND = shader_kind::ToonSpecular;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;

struct Light {
	glm::vec4 position;       // Позиция источника света
	glm::vec3 spotDirection;  // Направление прожектора
	float spotCosCutoff;      // Косинус угла отсечения
	float spotExponent;       // Коэффициент экспоненциального затухания
	glm::vec3 attenuation;    // Коэффициенты затухания (constant, linear, quadratic)
	glm::vec4 ambient;        // Фоновая составляющая
	glm::vec4 diffuse;        // Рассеянная составляющая
	glm::vec4 specular;       // Зеркальная составляющая
};

Light light = {
	glm::vec4(0.0f, 5.0f, 0.0f, 1.0f),  // Позиция прожектора (например, на высоте 5, по оси Y)
	glm::vec3(0.0f, -1.0f, 0.0f),  // Направление светового луча вниз по оси Y
	cos(glm::radians(20.0f)),  // Угол отсечения 30 градусов (косинус угла отсечения)
	20.0f,  // Коэффициент экспоненциального затухания (можно регулировать для более мягкого или резкого падения света)
	glm::vec3(1.0f, 0.1f, 0.01f),  // Коэффициенты затухания: (constant, linear, quadratic)
	glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),  // Фоновая составляющая (слабое освещение)
	glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),  // Рассеянная составляющая (освещает объекты белым светом)
	glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)  // Зеркальная составляющая (белый свет для отражений)
};

// ID шейдерной программы
GLuint Program;

void InitShader() {
	if constexpr (SHADER_KIND == shader_kind::Phong) {
		if constexpr (LIGHT_KIND == light_kind::PointLightSource)
			Program = load_shaders("shaders/phong_point.vert", "shaders/phong_point.frag");
		if constexpr (LIGHT_KIND == light_kind::DirLightSource)
			Program = load_shaders("shaders/phong_dir.vert", "shaders/phong_dir.frag");
		if constexpr (LIGHT_KIND == light_kind::Spotlight)
			Program = load_shaders("shaders/phong_spot.vert", "shaders/phong_spot.frag");
	}
	else if constexpr (SHADER_KIND == shader_kind::OrenNayar) {
		if constexpr (LIGHT_KIND == light_kind::PointLightSource)
			Program = load_shaders("shaders/phong_point.vert", "shaders/oren_nayar_point.frag");
		if constexpr (LIGHT_KIND == light_kind::DirLightSource)
			Program = load_shaders("shaders/phong_dir.vert", "shaders/oren_nayar_dir.frag");
		if constexpr (LIGHT_KIND == light_kind::Spotlight)
			Program = load_shaders("shaders/phong_spot.vert", "shaders/oren_nayar_spot.frag");
	}
	else if constexpr (SHADER_KIND == shader_kind::Toon) {
		if constexpr (LIGHT_KIND == light_kind::PointLightSource)
			Program = load_shaders("shaders/phong_point.vert", "shaders/toon_point.frag");
		if constexpr (LIGHT_KIND == light_kind::DirLightSource)
			Program = load_shaders("shaders/phong_dir.vert", "shaders/toon_dir.frag");
		if constexpr (LIGHT_KIND == light_kind::Spotlight)
			Program = load_shaders("shaders/phong_spot.vert", "shaders/toon_spot.frag");
	}
	else if constexpr (SHADER_KIND == shader_kind::ToonSpecular) {
		if constexpr (LIGHT_KIND == light_kind::PointLightSource)
			Program = load_shaders("shaders/phong_point.vert", "shaders/toon_spec_point.frag");
		if constexpr (LIGHT_KIND == light_kind::DirLightSource)
			Program = load_shaders("shaders/phong_dir.vert", "shaders/toon_spec_dir.frag");
		if constexpr (LIGHT_KIND == light_kind::Spotlight)
			Program = load_shaders("shaders/phong_spot.vert", "shaders/toon_spec_spot.frag");
	}
}



void Init() {
	// Шейдеры
	InitShader();
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.5, 0.5, 0.5, 0.0);
	model0 = Model(model_path, texture_path);
	model1 = Model(model_path1, texture_path1);
	model2 = Model(model_path2, texture_path2);
	ground_model = Model(model_path4, texture_path4);
	tree_model = Model(model_path3, texture_path3);
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

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 8.0f, -1.5f));
	model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	model0.display_model(Program);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 10.0f, 3.0f));
	model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 1.0f));
	normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	model1.display_model(Program);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.16f, 0.16f, 0.16f));
	normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	model2.display_model(Program);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
	model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
	model = glm::rotate(model, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	model2.display_model(Program);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, -2.0f));
	model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	tree_model.display_model(Program);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, -3.0f));
	model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	tree_model.display_model(Program);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.0f, 3.5f));
	model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	tree_model.display_model(Program);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 3.5f));
	model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	tree_model.display_model(Program);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -10.0f, 0.0f));
	model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	ground_model.display_model(Program);


	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.viewProjection"), 1, GL_FALSE, glm::value_ptr(projection * view));
	glUniform3fv(glGetUniformLocation(Program, "transform.viewPosition"), 1, glm::value_ptr(cameraPos));

	glUniform4fv(glGetUniformLocation(Program, "light.position"), 1, glm::value_ptr(light.position));
	glUniform4fv(glGetUniformLocation(Program, "light.ambient"), 1, glm::value_ptr(light.ambient));
	glUniform4fv(glGetUniformLocation(Program, "light.diffuse"), 1, glm::value_ptr(light.diffuse));
	glUniform4fv(glGetUniformLocation(Program, "light.specular"), 1, glm::value_ptr(light.specular));
	if constexpr (LIGHT_KIND != light_kind::DirLightSource)
		glUniform3fv(glGetUniformLocation(Program, "light.attenuation"), 1, glm::value_ptr(light.attenuation));

	if constexpr (LIGHT_KIND == light_kind::Spotlight) {
		glUniform3fv(glGetUniformLocation(Program, "light.spotDirection"), 1, glm::value_ptr(light.spotDirection)); // Направление прожектора
		glUniform1f(glGetUniformLocation(Program, "light.spotCosCutoff"), light.spotCosCutoff); // Косинус угла отсечения
		glUniform1f(glGetUniformLocation(Program, "light.spotExponent"), light.spotExponent); // Экспоненциальное затухание
	}

	glUniform1i(glGetUniformLocation(Program, "material.texture"), 0); // Текстура привязана к текстурному блоку 0
	glUniform4f(glGetUniformLocation(Program, "material.ambient"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(glGetUniformLocation(Program, "material.diffuse"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(glGetUniformLocation(Program, "material.specular"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(glGetUniformLocation(Program, "material.emission"), 0.0f, 0.0f, 0.0f, 1.0f);
	glUniform1f(glGetUniformLocation(Program, "material.shininess"), 32.0f);

	if constexpr (SHADER_KIND == shader_kind::OrenNayar)
		glUniform1f(glGetUniformLocation(Program, "roughness"), 0.3f); // От 0 до 1

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
	constexpr float cameraSpeed = 0.5f;
	float cameraShiftScale = 1.0f;
	constexpr float rotationSpeed = 0.5f;
	constexpr float lightSpeed = 0.2f;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) cameraShiftScale = 3.0f;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) cameraPos += cameraSpeed * cameraFront * cameraShiftScale;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) cameraPos -= cameraSpeed * cameraFront * cameraShiftScale;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * cameraShiftScale;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * cameraShiftScale;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) pitch += rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) pitch -= rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) yaw -= rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) yaw += rotationSpeed;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::J)) light.position[0] += lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::N)) light.position[0] -= lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::B)) light.position[1] += lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::M)) light.position[1] -= lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::H)) light.position[2] += lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::K)) light.position[2] -= lightSpeed;

	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;
}

int main() {
	sf::Window window(sf::VideoMode(900, 900), "My OpenGL window", sf::Style::Default, sf::ContextSettings(24));
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