#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

void check_compile_errors(GLuint shader_ID, std::string type)
{
	GLint status;
	GLchar info_log[1024];
	if (type == "PROGRAM")
	{
		glGetProgramiv(shader_ID, GL_LINK_STATUS, &status);
		if (!status)
		{
			glGetProgramInfoLog(shader_ID, 1024, NULL, info_log);
			std::cout << "Linking error occurred, type: " << type << "\n" << info_log << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
	else
	{
		glGetShaderiv(shader_ID, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			glGetShaderInfoLog(shader_ID, 1024, NULL, info_log);
			std::cout << "Shader compilation error occurred, type: " << type << "\n" << info_log << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
}

GLuint load_shaders(const char* vertex_shad_path, const char* fragment_shad_path)
{
	std::string vertex_code;
	std::string fragment_code;

	std::ifstream vert_shad_file;
	std::ifstream frag_shad_file;

	vert_shad_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	frag_shad_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		vert_shad_file.open(vertex_shad_path);
		frag_shad_file.open(fragment_shad_path);
		std::stringstream vert_shader_stream, frag_shader_stream;

		vert_shader_stream << vert_shad_file.rdbuf();
		frag_shader_stream << frag_shad_file.rdbuf();

		vert_shad_file.close();
		frag_shad_file.close();

		vertex_code = vert_shader_stream.str();
		fragment_code = frag_shader_stream.str();
	}
	catch (std::ifstream::failure& err)
	{
		std::cout << "Error occurred while reading shader file: " << err.what() << std::endl;
	}
	const char* vertex_code_char = vertex_code.c_str();
	const char* fragment_code_char = fragment_code.c_str();
	unsigned int vertex, fragment;

	// vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertex_code_char, NULL);
	glCompileShader(vertex);
	check_compile_errors(vertex, "VERTEX");

	// fragment Shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragment_code_char, NULL);
	glCompileShader(fragment);
	check_compile_errors(fragment, "FRAGMENT");

	// shader Program
	GLuint ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	check_compile_errors(ID, "PROGRAM");

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return ID;
}

#endif