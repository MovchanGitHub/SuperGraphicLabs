#define _USE_MATH_DEFINES
#include <cmath>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>


enum class Polygon {TRIANGLE, SQUARE, FAN, PENTAGON};

// Здесь выбор, что рисуется
constexpr Polygon POLYGON_TO_DRAW = Polygon::PENTAGON;
constexpr GLuint FAN_VERTICES = 6; // > 3

// Исходный код вершинного шейдера
const char* VertexShaderSource = R"(
 #version 330 core
 in vec2 coord;
 void main() {
 gl_Position = vec4(coord.x, coord.y, 0.0, 1.25);
 }
)";

// Исходный код фрагментного шейдера из второго задания
const char* FragShaderSource_ConstantColorInShader = R"(
 #version 330 core
 out vec4 color;
 void main() {
 color = vec4(0, 1, 0, 1);
 }
)";

// ID шейдерной программы
GLuint Program;
// ID атрибута
GLint Attrib_vertex;
// ID Vertex Buffer Object
GLuint VBO;

struct Vertex {
	GLfloat x;
	GLfloat y;
};

void ShaderLog(unsigned int shader)
{
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
	glShaderSource(fShader, 1, &FragShaderSource_ConstantColorInShader, NULL);
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
	Attrib_vertex = glGetAttribLocation(Program, attr_name);
	if (Attrib_vertex == -1) {
		std::cout << "could not bind attrib " << attr_name << std::endl;
		return;
	}
	//checkOpenGLerror();
}


void InitVBO() {
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Вершины наших полигонов
	// Передаем вершины в буфер
	if constexpr (POLYGON_TO_DRAW == Polygon::TRIANGLE) {
		Vertex triangle[3] = { { -1.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, -1.0f } };
		glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
	}
	else if constexpr (POLYGON_TO_DRAW == Polygon::SQUARE) {
		Vertex square[4] = { { -1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f } };
		glBufferData(GL_ARRAY_BUFFER, sizeof(square), square, GL_STATIC_DRAW);
	}
	else if constexpr (POLYGON_TO_DRAW == Polygon::FAN) {
		Vertex fan[FAN_VERTICES];
		fan[0].x = 0;
		fan[0].y = -0.5;
		GLdouble alpha = 0;
		constexpr GLdouble delta = M_PI / (FAN_VERTICES - 2);
		for (GLuint i = 1; i < FAN_VERTICES; ++i) {
			fan[i].x = std::cos(alpha);
			fan[i].y = std::sin(alpha) - 0.5;
			alpha += delta;
		}
		glBufferData(GL_ARRAY_BUFFER, sizeof(fan), fan, GL_STATIC_DRAW);
	}
	else if constexpr (POLYGON_TO_DRAW == Polygon::PENTAGON) {
		Vertex pengaton[5];
		GLdouble alpha = -3 * M_PI / 10;
		constexpr GLdouble delta = 2 * M_PI / 5;
		for (GLuint i = 0; i < 5; ++i) {
			pengaton[i].x = std::cos(alpha);
			pengaton[i].y = std::sin(alpha);
			alpha += delta;
		}
		glBufferData(GL_ARRAY_BUFFER, sizeof(pengaton), pengaton, GL_STATIC_DRAW);
	}

	//checkOpenGLerror(); //Пример функции есть в лабораторной
	// Проверка ошибок OpenGL, если есть, то вывод в консоль тип ошибки
}

void Init() {
	// Шейдеры
	InitShader();
	// Вершинный буфер
	InitVBO();
}

void Draw() {
	glUseProgram(Program); // Устанавливаем шейдерную программу текущей
	glEnableVertexAttribArray(Attrib_vertex); // Включаем массив атрибутов
	glBindBuffer(GL_ARRAY_BUFFER, VBO); // Подключаем VBO
	// сообщаем OpenGL как он должен интерпретировать вершинные данные.
	glVertexAttribPointer(Attrib_vertex, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0); // Отключаем VBO

	if constexpr (POLYGON_TO_DRAW == Polygon::TRIANGLE)
		glDrawArrays(GL_TRIANGLES, 0, 3); // Передаем данные на видеокарту(рисуем)
	else if constexpr (POLYGON_TO_DRAW == Polygon::SQUARE)
		glDrawArrays(GL_POLYGON, 0, 4); // Передаем данные на видеокарту(рисуем)
	else if constexpr (POLYGON_TO_DRAW == Polygon::FAN)
		glDrawArrays(GL_TRIANGLE_FAN, 0, FAN_VERTICES); // Передаем данные на видеокарту(рисуем)
	else if constexpr (POLYGON_TO_DRAW == Polygon::PENTAGON)
		glDrawArrays(GL_POLYGON, 0, 5); // Передаем данные на видеокарту(рисуем)

	glDisableVertexAttribArray(Attrib_vertex); // Отключаем массив атрибутов
	glUseProgram(0); // Отключаем шейдерную программу

	//checkOpenGLerror();
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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		Draw();
		window.display();
	}
	Release();
	return 0;
}