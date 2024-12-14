#define _USE_MATH_DEFINES
#include <cmath>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SOIL.h"

bool mode = 0;

const char* VertexColorShaderSource = R"(
 #version 330 core
 in vec3 coord;
 uniform mat4 model;
 uniform mat4 view;
 uniform mat4 projection;
 in vec4 color;
 in vec2 tex_coord;
 out vec4 vertex_color;
 out vec2 out_tex_coord;
 

 void main() {
    vertex_color = color;
	out_tex_coord = vec2(tex_coord.x, 1.0f - tex_coord.y);
    gl_Position = projection * view * model * vec4(coord, 1.0);
 }
)";

const char* FragShaderSource_TextureMix = R"(
 #version 330 core
 in vec4 vertex_color;
 in vec2 out_tex_coord;
 out vec4 color;
 uniform sampler2D texture1;
 uniform float mix_value;

 void main() {
    vec4 tex_color = texture(texture1, out_tex_coord);
    color = mix(tex_color, vertex_color, mix_value);
 }
)";

const char* FragShaderSource_TextureMix2 = R"(
 #version 330 core
 in vec4 vertex_color;
 in vec2 out_tex_coord;
 out vec4 color;
 uniform sampler2D texture1;
 uniform sampler2D texture2;
 uniform float mix_value;

 void main() {
    vec4 tex_color_1 = texture(texture1, out_tex_coord);
	vec4 tex_color_2 = texture(texture2, out_tex_coord);
    color = mix(tex_color_1, tex_color_2, mix_value);
 }
)";


GLuint Program;

GLint Attrib_vertex_coords;
GLint Attrib_vertex_color;
GLint Attrib_tex_coords;

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
	GLfloat u = 0;
	GLfloat v = 0;
};

Vertex cubeVertices[8];

void GenerateCubeVertices() {
	cubeVertices[0] = { -1, -1, -1, {1.0f, 0.0f, 0.0f, 1.0f}, 0.0f, 0.0f };
	cubeVertices[1] = { 1, -1, -1, {0.0f, 1.0f, 0.0f, 1.0f}, 1.0f, 0.0f };
	cubeVertices[2] = { 1,  1, -1, {0.0f, 0.0f, 1.0f, 1.0f}, 1.0f, 1.0f };
	cubeVertices[3] = { -1,  1, -1, {1.0f, 1.0f, 0.0f, 1.0f}, 0.0f, 1.0f };
	cubeVertices[4] = { -1, -1,  1, {1.0f, 0.0f, 1.0f, 1.0f}, 0.0f, 0.0f };
	cubeVertices[5] = { 1, -1,  1, {0.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.0f };
	cubeVertices[6] = { 1,  1,  1, {1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 1.0f };
	cubeVertices[7] = { -1,  1,  1, {0.0f, 0.0f, 0.0f, 1.0f}, 0.0f, 1.0f };
}

GLuint texture1;
GLuint texture2;
void LoadTexture(const char* filepath1, const char* filepath2) {
	int width, height;
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	unsigned char* image = SOIL_load_image(filepath1, &width, &height, 0, SOIL_LOAD_RGB);
	if (image) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);
		SOIL_free_image_data(image);
		std::cout << "Texture loaded successfully: " << filepath1 << std::endl;
	}
	else {
		std::cout << "Failed to load texture: " << filepath1 << std::endl;
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &texture2);
	glBindTexture(GL_TEXTURE_2D, texture2);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	image = SOIL_load_image(filepath2, &width, &height, 0, SOIL_LOAD_RGB);
	if (image) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);
		SOIL_free_image_data(image);
		std::cout << "Texture loaded successfully: " << filepath2 << std::endl;
	}
	else {
		std::cout << "Failed to load texture: " << filepath2 << std::endl;
	}
	glBindTexture(GL_TEXTURE_2D, 0);
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

void InitShader() {
	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vShader, 1, &VertexColorShaderSource, NULL);
	glCompileShader(vShader);
	std::cout << "vertex shader \n";
	ShaderLog(vShader);

	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
	//Заменить шейдер для задания 2ыы
	glShaderSource(fShader, 1, &FragShaderSource_TextureMix2, NULL);
	glCompileShader(fShader);
	std::cout << "fragment shader \n";
	ShaderLog(fShader);

	Program = glCreateProgram();
	glAttachShader(Program, vShader);
	glAttachShader(Program, fShader);
	glLinkProgram(Program);

	int link_ok;
	glGetProgramiv(Program, GL_LINK_STATUS, &link_ok);
	if (!link_ok) {
		std::cout << "error attach shaders \n";
		return;
	}
	const char* attr_name = "coord";
	Attrib_vertex_coords = glGetAttribLocation(Program, attr_name);
	if (Attrib_vertex_coords == -1) {
		std::cout << "could not bind attrib " << attr_name << std::endl;
		return;
	}
	//Раскоментировать для задания 2
	/*
	attr_name = "color";
	Attrib_vertex_color = glGetAttribLocation(Program, attr_name);
	if (Attrib_vertex_color == -1) {
		std::cout << "could not bind attrib " << attr_name << std::endl;
		return;
	}
	*/
	attr_name = "tex_coord";
	Attrib_tex_coords = glGetAttribLocation(Program, attr_name);
	if (Attrib_tex_coords == -1) {
		std::cout << "could not bind attrib " << attr_name << std::endl;
		return;
	}
}

void InitVBO() {
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	const Vertex& p1 = cubeVertices[0];
	const Vertex& p2 = cubeVertices[1];
	const Vertex& p3 = cubeVertices[2];
	const Vertex& p4 = cubeVertices[3];
	const Vertex& p5 = cubeVertices[4];
	const Vertex& p6 = cubeVertices[5];
	const Vertex& p7 = cubeVertices[6];
	const Vertex& p8 = cubeVertices[7];

	Vertex cube[] = {
		p1, p2, p3, p3, p4, p1,
		p5, p6, p7, p7, p8, p5,
		{p1.x, p1.y, p1.z, p1.c, 0.0f, 0.0f}, {p5.x, p5.y, p5.z, p5.c, 1.0f, 0.0f},
		{p8.x, p8.y, p8.z, p8.c, 1.0f, 1.0f}, {p8.x, p8.y, p8.z, p8.c, 1.0f, 1.0f},
		{p4.x, p4.y, p4.z, p4.c, 0.0f, 1.0f}, {p1.x, p1.y, p1.z, p1.c, 0.0f, 0.0f},
		{p2.x, p2.y, p2.z, p2.c, 0.0f, 0.0f}, {p6.x, p6.y, p6.z, p6.c, 1.0f, 0.0f},
		{p7.x, p7.y, p7.z, p7.c, 1.0f, 1.0f}, {p7.x, p7.y, p7.z, p7.c, 1.0f, 1.0f},
		{p3.x, p3.y, p3.z, p3.c, 0.0f, 1.0f}, {p2.x, p2.y, p2.z, p2.c, 0.0f, 0.0f},
		{p4.x, p4.y, p4.z, p4.c, 0.0f, 0.0f}, {p3.x, p3.y, p3.z, p3.c, 1.0f, 0.0f},
		{p7.x, p7.y, p7.z, p7.c, 1.0f, 1.0f}, {p7.x, p7.y, p7.z, p7.c, 1.0f, 1.0f},
		{p8.x, p8.y, p8.z, p8.c, 0.0f, 1.0f}, {p4.x, p4.y, p4.z, p4.c, 0.0f, 0.0f},
		{p1.x, p1.y, p1.z, p1.c, 0.0f, 0.0f}, {p2.x, p2.y, p2.z, p2.c, 1.0f, 0.0f},
		{p6.x, p6.y, p6.z, p6.c, 1.0f, 1.0f}, {p6.x, p6.y, p6.z, p6.c, 1.0f, 1.0f},
		{p5.x, p5.y, p5.z, p5.c, 0.0f, 1.0f}, {p1.x, p1.y, p1.z, p1.c, 0.0f, 0.0f}
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glEnableVertexAttribArray(Attrib_vertex_coords);
	glVertexAttribPointer(Attrib_vertex_coords, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	glEnableVertexAttribArray(Attrib_vertex_color);
	glVertexAttribPointer(Attrib_vertex_color, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, c));
	glEnableVertexAttribArray(Attrib_tex_coords);
	glVertexAttribPointer(Attrib_tex_coords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, u));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

glm::mat4 model;

void Init() {
    GenerateCubeVertices();
    InitShader();
    InitVBO();
    glEnable(GL_DEPTH_TEST);
	LoadTexture("texture1.jpg", "texture2.jpg");

    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

    glUseProgram(Program);
    glUniformMatrix4fv(glGetUniformLocation(Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1f(glGetUniformLocation(Program, "mix_value"), 0.5f);
    glUseProgram(0);
}


float angleX = 0.0f;
float angleY = 0.0f;
float mixValue = 0.5f;

void Draw() {
	glUseProgram(Program);

	glm::mat4 model = glm::rotate(glm::mat4(1.0f), angleX, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, angleY, glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1f(glGetUniformLocation(Program, "mix_value"), mixValue);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1);
	glUniform1i(glGetUniformLocation(Program, "texture1"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2);
	glUniform1i(glGetUniformLocation(Program, "texture2"), 1);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glEnableVertexAttribArray(Attrib_vertex_coords);
	glVertexAttribPointer(Attrib_vertex_coords, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	//Раскоментировать для задания 2
	//glEnableVertexAttribArray(Attrib_vertex_color);
	//glVertexAttribPointer(Attrib_vertex_color, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, c));
	glEnableVertexAttribArray(Attrib_tex_coords);
	glVertexAttribPointer(Attrib_tex_coords, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, u));


	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glDisableVertexAttribArray(Attrib_vertex_coords);
	//Раскоментировать для задания 2
	//glDisableVertexAttribArray(Attrib_vertex_color);
	glDisableVertexAttribArray(Attrib_tex_coords);
	glUseProgram(0);
}

void ReleaseVBO() {
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	glDeleteBuffers(1, &VBO);
}

void ReleaseShader() {
	glUseProgram(0);
	glDeleteProgram(Program);
}

void Release() {
	ReleaseShader();
	ReleaseVBO();
}


void HandleKeyboardInput() {
	constexpr float rotationSpeed = 0.05f;
	constexpr float mixSpeed = 0.01f;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) angleX -= rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) angleX += rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) angleY -= rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) angleY += rotationSpeed;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) mixValue += mixSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) mixValue -= mixSpeed;

	if (mixValue < 0.0f) mixValue = 0.0f;
	if (mixValue > 1.0f) mixValue = 1.0f;
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