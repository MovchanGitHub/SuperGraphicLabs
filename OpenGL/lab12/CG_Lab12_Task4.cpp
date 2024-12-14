#define _USE_MATH_DEFINES
#include <cmath>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <random>

constexpr GLuint circle_vert_count = 360;
constexpr double to_degree = M_PI / 180.0;

// Исходный код вершинного шейдера
const char* VertexShaderSource = R"(
 #version 330 core
 in vec2 coord;
 in vec4 color;

 out vec4 vertex_color;

 uniform float scale_x;
 uniform float scale_y;

 void main() {
	vertex_color = color;
	gl_Position = vec4(coord.x * scale_x, coord.y * scale_y, 0.0, 1.0);
 }
)";

// Исходный код фрагментного шейдера
const char* FragShaderSource = R"(
 #version 330 core
 in vec4 vertex_color;
 
 out vec4 color;
 
 void main() {
	color = vertex_color;
 }
)";

// ID шейдерной программы
GLuint Program;
// ID атрибутов
GLint Attrib_vertex1;
GLint Attrib_vertex2;
// ID Vertex Buffer Object
GLuint VBO;

float scale_x = 1.0;
float scale_y = 1.0;

struct Color {
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat alpha;
};

struct Vertex {
	GLfloat x;
	GLfloat y;
	Color c;
};

Color hsv_to_rgb(float hue, float saturation = 100.0f, float value = 100.0f) {
	float satur_norm = saturation / 100.0f, val_norm = value / 100.0f;
	int h_i = (int)std::floor(hue / 60) % 6;
	float f = hue / 60 - std::floor(hue / 60), p = val_norm * (1 - satur_norm);
	float q = val_norm * (1 - f * satur_norm), t = val_norm * (1 - (1 - f) * satur_norm);
	switch (h_i) {
	case 1:
		return { q, val_norm, p, 1.0f };
	case 2:
		return { p, val_norm, t, 1.0f };
	case 3:
		return { p, q, val_norm, 1.0f };
	case 4:
		return { t, p, val_norm, 1.0f };
	case 5:
		return { val_norm, p, q, 1.0f };
	default:
		return { val_norm, t, p, 1.0f };
	}
}

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
	glShaderSource(fShader, 1, &FragShaderSource, NULL);
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
	// Вытягиваем ID атрибута из собранной программы
	const char* attr_name = "coord"; //имя в шейдере
	Attrib_vertex1 = glGetAttribLocation(Program, attr_name);
	if (Attrib_vertex1 == -1) {
		std::cout << "could not bind attrib " << attr_name << std::endl;
		return;
	}
	attr_name = "color"; //имя в шейдере
	Attrib_vertex2 = glGetAttribLocation(Program, attr_name);
	if (Attrib_vertex2 == -1) {
		std::cout << "could not bind attrib " << attr_name << std::endl;
		return;
	}
}

void InitVBO() {
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	
	Vertex circle[circle_vert_count * 3];
	for (int i = 0; i < circle_vert_count; ++i) {
		circle[i * 3].x = 0.5f * std::cos(i * (360.0f / circle_vert_count) * to_degree);
		circle[i * 3].y = 0.5f * std::sin(i * (360.0f / circle_vert_count) * to_degree);
		circle[i * 3].c = hsv_to_rgb(i % 360);

		circle[i * 3 + 1].x = 0.5f * std::cos((i + 1) * (360.0f / circle_vert_count) * to_degree);
		circle[i * 3 + 1].y = 0.5f * std::sin((i + 1) * (360.0f / circle_vert_count) * to_degree);
		circle[i * 3 + 1].c = hsv_to_rgb((i + 1) % 360);

		circle[i * 3 + 2].x = 0.0f;
		circle[i * 3 + 2].y = 0.0f;
		circle[i * 3 + 2].c = { 1.0f, 1.0f, 1.0f, 1.0f };
	}
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(circle), circle, GL_STATIC_DRAW);
}

void Init() {
	// Шейдеры
	InitShader();
	// Вершинный буфер
	InitVBO();
}

void Draw() {
	glUseProgram(Program); // Устанавливаем шейдерную программу текущей

	glUniform1f(glGetUniformLocation(Program, "scale_x"), scale_x); // Передаем в шейдер значение цвета через uniform-переменную
	glUniform1f(glGetUniformLocation(Program, "scale_y"), scale_y); // Передаем в шейдер значение цвета через uniform-переменную

	glBindBuffer(GL_ARRAY_BUFFER, VBO); // Подключаем VBO
	// сообщаем OpenGL как он должен интерпретировать вершинные данные.
	glEnableVertexAttribArray(Attrib_vertex1);
	glVertexAttribPointer(Attrib_vertex1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glEnableVertexAttribArray(Attrib_vertex2);
	glVertexAttribPointer(Attrib_vertex2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, c));
	glBindBuffer(GL_ARRAY_BUFFER, 0); // Отключаем VBO

	glDrawArrays(GL_TRIANGLES, 0, circle_vert_count * 3);

	glDisableVertexAttribArray(Attrib_vertex1); // Отключаем массив атрибутов
	glDisableVertexAttribArray(Attrib_vertex2); // Отключаем массив атрибутов
	glUseProgram(0); // Отключаем шейдерную программу
}


// Освобождение буфера
void ReleaseVBO() {
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	glDeleteBuffers(1, &VBO);
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
	ReleaseVBO();
}

void HandleKeyboardInput() {
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) scale_y += 0.05;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) scale_y -= 0.05;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) scale_x -= 0.05;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) scale_x += 0.05;
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