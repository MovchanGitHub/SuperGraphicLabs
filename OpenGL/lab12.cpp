#define _USE_MATH_DEFINES
#include <cmath>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <random>


enum class Polygon { TRIANGLE, SQUARE, FAN, PENTAGON, TETRAHEDRON };
enum class Mode { CONST_COLOR, UNIFORM_COLOR, GRADIENT };

// Здесь выбор, что рисуется
constexpr Polygon DRAWING_OBJECT = Polygon::TETRAHEDRON;
constexpr GLuint FAN_VERTICES = 10; // > 2
constexpr Mode DRAW_MODE = Mode::GRADIENT;

GLfloat user_color[4] = { 0.9f, 0.0f, 0.25f, 1.0f };

// Исходный код вершинного шейдера
const char* VertexShaderSource = R"(
 #version 330 core
 in vec3 coord;
 void main() {
	gl_Position = vec4(coord, 1.25);
 }
)";

// Исходный код вершинного шейдера с аттрибутом цвета
const char* VertexColorShaderSource = R"(
 #version 330 core
 in vec3 coord;
 uniform vec3 offset;
 in vec4 color;
 out vec4 vertex_color;

 mat3 rotateY(float angle) {
	return mat3(
      cos(angle), 0.0, sin(angle),
      0.0, 1.0, 0.0,
      -sin(angle), 0.0, cos(angle)
    );
 }

 void main() {
	vertex_color = color;
	gl_Position = vec4(rotateY(0.23) * coord + offset, 1.25);
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

// Исходный код фрагментного шейдера из третьего задания
const char* FragShaderSource_UniformColorInShader = R"(
 #version 330 core
 uniform vec4 user_color;
 out vec4 color;
 void main() {
	color = user_color;
 }
)";

// Исходный код фрагментного шейдера из четвертого задания
const char* FragShaderSource_GradientInShader = R"(
 #version 330 core
 in vec4 vertex_color;
 out vec4 color;
 void main() {
	color = vertex_color;
 }
)";

// ID шейдерной программы
GLuint Program;
// ID атрибута
GLint Attrib_vertex_coords;
GLint Attrib_vertex_color;
// ID Vertex Buffer Object
GLuint VBO;

struct Color {
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat alpha;
};

struct Vertex {
	GLfloat x = 0;
	GLfloat y = 0;
	GLfloat z = 0;
	Color c;
};

Vertex tetrahedronVertices[4];


void GenerateTetrahedronVertices() {
	tetrahedronVertices[0] = { -1,  1,  0, {0.0f, 0.0f, 1.0f, 1.0f} };
	tetrahedronVertices[1] = { 1,  1,  0, {1.0f, 0.0f, 0.0f, 1.0f} };
	tetrahedronVertices[2] = { 0, -1,  0, {0.0f, 1.0f, 0.0f, 1.0f} };
	tetrahedronVertices[3] = { 0,  0, -1, {1.0f, 1.0f, 1.0f, 1.0f} };
}

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

std::vector<Color> gen_random_colors(int count) {
	std::random_device rd;
	std::mt19937 mt_eng(rd());
	std::uniform_real_distribution<> un_distr(0.0, 1.0);
	std::vector<Color> res;
	for (int i = 0; i < count; ++i)
		res.push_back({ (GLfloat)un_distr(mt_eng), (GLfloat)un_distr(mt_eng), (GLfloat)un_distr(mt_eng), (GLfloat)un_distr(mt_eng) });
	return res;
}

void InitShader() {
	// Создаем вершинный шейдер
	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	// Передаем исходный код
	if constexpr (DRAW_MODE == Mode::GRADIENT)
		glShaderSource(vShader, 1, &VertexColorShaderSource, NULL);
	else
		glShaderSource(vShader, 1, &VertexShaderSource, NULL);
	// Компилируем шейдер
	glCompileShader(vShader);
	std::cout << "vertex shader \n";
	// Функция печати лога шейдера
	ShaderLog(vShader);
	// Создаем фрагментный шейдер
	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Передаем исходный код
	if constexpr (DRAW_MODE == Mode::CONST_COLOR)
		glShaderSource(fShader, 1, &FragShaderSource_ConstantColorInShader, NULL);
	else if constexpr (DRAW_MODE == Mode::UNIFORM_COLOR)
		glShaderSource(fShader, 1, &FragShaderSource_UniformColorInShader, NULL);
	else if constexpr (DRAW_MODE == Mode::GRADIENT)
		glShaderSource(fShader, 1, &FragShaderSource_GradientInShader, NULL);
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
	Attrib_vertex_coords = glGetAttribLocation(Program, attr_name);
	if (Attrib_vertex_coords == -1) {
		std::cout << "could not bind attrib " << attr_name << std::endl;
		return;
	}
	attr_name = "color"; //имя в шейдере
	Attrib_vertex_color = glGetAttribLocation(Program, attr_name);
	if (Attrib_vertex_color == -1) {
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
	if constexpr (DRAWING_OBJECT == Polygon::TRIANGLE) {
		std::vector<Color> vert_colors = gen_random_colors(3);
		Vertex triangle[3] = {
			{ -1.0f, -1.0f, 0.0f, vert_colors[0] }
		, {0.0f, 1.0f, 0.0f,  vert_colors[1] }
		, {1.0f, -1.0f, 0.0f, vert_colors[2] }
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
	}
	else if constexpr (DRAWING_OBJECT == Polygon::SQUARE) {
		std::vector<Color> vert_colors = gen_random_colors(4);
		Vertex square[4] = {
			{ -1.0f, -1.0f, 0.0f, vert_colors[0] }
		, { -1.0f, 1.0f, 0.0f, vert_colors[1] }
		, { 1.0f, 1.0f, 0.0f, vert_colors[2] }
		, { 1.0f, -1.0f, 0.0f, vert_colors[3] }
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(square), square, GL_STATIC_DRAW);
	}
	else if constexpr (DRAWING_OBJECT == Polygon::FAN) {
		static_assert(FAN_VERTICES > 2, "FAN_VERTICES should be greater than 2.");
		std::vector<Color> vert_colors = gen_random_colors(FAN_VERTICES);
		Vertex fan[FAN_VERTICES];
		fan[0].x = 0;
		fan[0].y = -0.5;
		fan[0].c = vert_colors[0];
		GLdouble alpha = M_PI / 18;
		constexpr GLdouble delta = 8 * M_PI / 9 / (FAN_VERTICES - 2);
		for (GLuint i = 1; i < FAN_VERTICES; ++i) {
			fan[i].x = std::cos(alpha);
			fan[i].y = std::sin(alpha) - 0.5;
			alpha += delta;
			fan[i].c = vert_colors[i];
		}
		glBufferData(GL_ARRAY_BUFFER, sizeof(fan), fan, GL_STATIC_DRAW);
	}
	else if constexpr (DRAWING_OBJECT == Polygon::PENTAGON) {
		std::vector<Color> vert_colors = gen_random_colors(5);
		Vertex pengaton[5];
		GLdouble alpha = -3 * M_PI / 10;
		constexpr GLdouble delta = 2 * M_PI / 5;
		for (GLuint i = 0; i < 5; ++i) {
			pengaton[i].x = std::cos(alpha);
			pengaton[i].y = std::sin(alpha);
			alpha += delta;
			pengaton[i].c = vert_colors[i];
		}
		glBufferData(GL_ARRAY_BUFFER, sizeof(pengaton), pengaton, GL_STATIC_DRAW);
	}
	else if constexpr (DRAWING_OBJECT == Polygon::TETRAHEDRON) {
		const Vertex& p1 = tetrahedronVertices[0];
		const Vertex& p2 = tetrahedronVertices[1];
		const Vertex& p3 = tetrahedronVertices[2];
		const Vertex& p4 = tetrahedronVertices[3];
		Vertex tetrahedron[] = {
			p1, p2, p3,
			p1, p2, p4,
			p1, p3, p4,
			p2, p3, p4,
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(tetrahedron), tetrahedron, GL_DYNAMIC_DRAW);
	}

	//checkOpenGLerror(); //Пример функции есть в лабораторной
	// Проверка ошибок OpenGL, если есть, то вывод в консоль тип ошибки
}

void Init() {
	GenerateTetrahedronVertices();
	// Шейдеры
	InitShader();
	// Вершинный буфер
	InitVBO();
	// Включаем проверку глубины
	glEnable(GL_DEPTH_TEST);
}

GLfloat vertex_offset[3] = { 0.0f, 0.0f, 0.0f };

void Draw() {
	glUseProgram(Program); // Устанавливаем шейдерную программу текущей

	if constexpr (DRAW_MODE == Mode::UNIFORM_COLOR) {
		glUniform4fv(glGetUniformLocation(Program, "user_color"), 1, user_color); // Передаем в шейдер значение цвета через uniform-переменную
	}
	else if constexpr (DRAWING_OBJECT == Polygon::TETRAHEDRON) {
		glUniform3fv(glGetUniformLocation(Program, "offset"), 1, vertex_offset);
	}

	//glEnableVertexAttribArray(Attrib_vertex); // Включаем массив атрибутов
	glBindBuffer(GL_ARRAY_BUFFER, VBO); // Подключаем VBO
	// сообщаем OpenGL как он должен интерпретировать вершинные данные.
	glEnableVertexAttribArray(Attrib_vertex_coords);
	glVertexAttribPointer(Attrib_vertex_coords, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	glEnableVertexAttribArray(Attrib_vertex_color);
	glVertexAttribPointer(Attrib_vertex_color, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, c));
	glBindBuffer(GL_ARRAY_BUFFER, 0); // Отключаем VBO

	if constexpr (DRAWING_OBJECT == Polygon::TRIANGLE)
		glDrawArrays(GL_TRIANGLES, 0, 3); // Передаем данные на видеокарту(рисуем)
	else if constexpr (DRAWING_OBJECT == Polygon::SQUARE)
		glDrawArrays(GL_POLYGON, 0, 4); // Передаем данные на видеокарту(рисуем)
	else if constexpr (DRAWING_OBJECT == Polygon::FAN)
		glDrawArrays(GL_TRIANGLE_FAN, 0, FAN_VERTICES); // Передаем данные на видеокарту(рисуем)
	else if constexpr (DRAWING_OBJECT == Polygon::PENTAGON)
		glDrawArrays(GL_POLYGON, 0, 5); // Передаем данные на видеокарту(рисуем)	
	else if constexpr (DRAWING_OBJECT == Polygon::TETRAHEDRON)
		glDrawArrays(GL_TRIANGLES, 0, 12); // Передаем данные на видеокарту(рисуем)

	glDisableVertexAttribArray(Attrib_vertex_coords); // Отключаем массив атрибутов
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

void HandleKeyboardInput() {
	constexpr float moveStep = 0.1f;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) vertex_offset[1] += moveStep;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) vertex_offset[1] -= moveStep;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) vertex_offset[0] -= moveStep;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) vertex_offset[0] += moveStep;
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