#ifndef MODEL_H
#define MODEL_H

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "glm/glm.hpp"

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 tex_coords;

	Vertex(float pos_x, float pos_y, float pos_z) {
		position = glm::vec3(pos_x, pos_y, pos_z);
		normal = {};
		tex_coords = {};
	}
};

struct Texture {
	GLuint id = -1;
	sf::Texture texture;
};

class Mesh {
	void setup_mesh() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords));
		glBindVertexArray(0);
	}

	void setup_texture(const std::string& text_path) {
		sf::Texture tex;
		tex.loadFromFile(text_path);
		tex.setRepeated(true);
		texture = { 0, tex };
	}

	void release() {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
		glDeleteVertexArrays(1, &VAO);
	}

public:
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	Texture texture;
	GLuint VAO, VBO, EBO;

	Mesh() = default;

	friend class Model;

	void display_mesh(GLuint shader_id) const {
		if (texture.id != -1) {
			glActiveTexture(GL_TEXTURE0);
			glUniform1i(glGetUniformLocation(shader_id, "tex"), 0);
			sf::Texture::bind(&texture.texture);
		}
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, (GLuint)indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		if (texture.id != -1)
			sf::Texture::bind(NULL);
	}
};

std::vector<std::string> split(const std::string& str, char sep = ' ') {
	std::vector<std::string> res;
	std::stringstream iss(str);
	std::string word;
	while (std::getline(iss, word, sep)) {
		if (word.empty())
			continue;
		res.push_back(word);
	}
	return res;
}

class Model
{
public:
	std::vector<Mesh> meshes;
	std::string dir;

	Model() = default;

	Model(std::string const& file_path, std::string const& tex_path = "") {
		load_model(file_path, tex_path);
	}

	void display_model(GLuint shader_id) {
		for (const Mesh& mesh : meshes)
			mesh.display_mesh(shader_id);
	}

	void release() {
		for (Mesh& mesh : meshes)
			mesh.release();
	}

private:
	Vertex process_vertex(const std::string& vert, const std::vector<glm::vec3>& vert_positions,
		const std::vector<glm::vec3>& vert_normals, const std::vector<glm::vec2>& vert_tex_coords) {
		GLuint vertex_ind, norm_ind, tex_coord_ind;
		std::istringstream iss(vert);
		
		iss >> vertex_ind;
		Vertex res_vert(vert_positions[--vertex_ind].x,
			vert_positions[vertex_ind].y, vert_positions[vertex_ind].z);
		char ch1 = iss.peek();
		if (ch1 == '/') {
			iss.ignore();
			ch1 = iss.peek();
			if (ch1 == '/') {
				iss.ignore();
				iss >> norm_ind;
				res_vert.normal = vert_normals[--norm_ind];
			}
			else if (isdigit(ch1)) {
				iss >> tex_coord_ind;
				res_vert.tex_coords = vert_tex_coords[--tex_coord_ind];
				ch1 = iss.peek();
				if (ch1 == '/') {
					iss.ignore();
					iss >> norm_ind;
					res_vert.normal = vert_normals[--norm_ind];
				}
			}
		}

		return res_vert;
	}

	void load_model(const std::string& file_name, const std::string& tex_path) {
		std::ifstream file(file_name);
		if (!file.is_open()) {
			std::cerr << "Failed to open file: " << file_name << std::endl;
			return;
		}

		bool is_new_mesh = false;
		std::vector<glm::vec3> vert_positions;
		std::vector<glm::vec3> vert_normals;
		std::vector<glm::vec2> vert_tex_coords;
		std::vector<GLuint> indices;
		Mesh cur_mesh;
		std::string line;
		
		while (std::getline(file, line)) {
			std::istringstream iss(line);
			std::string type;
			iss >> type;
			if (type.empty() || type[0] == '#') {
				continue;
			}
			if (type == "v") {
				if (is_new_mesh) {
					cur_mesh.indices = indices;
					meshes.push_back(cur_mesh);
					cur_mesh = Mesh();
					indices.clear();
					is_new_mesh = false;
				}
				float x, y, z;
				iss >> x >> y >> z;
				vert_positions.emplace_back(x, y, z);
			}
			else if (type == "vn") {
				float x, y, z;
				iss >> x >> y >> z;
				vert_normals.emplace_back(x, y, z);
			}
			else if (type == "vt") {
				double x, y;
				iss >> x >> y;
				vert_tex_coords.emplace_back(x, y);
			}
			else if (type == "f") {
				if (!is_new_mesh)
					is_new_mesh = true;
				auto spl = split(line);
				for (int i = 3; i < spl.size(); ++i) {
					cur_mesh.vertices.push_back(process_vertex(spl[1], vert_positions,
						vert_normals, vert_tex_coords));
					indices.push_back(indices.size());
					cur_mesh.vertices.push_back(process_vertex(spl[i - 1], vert_positions,
						vert_normals, vert_tex_coords));
					indices.push_back(indices.size());
					cur_mesh.vertices.push_back(process_vertex(spl[i], vert_positions,
						vert_normals, vert_tex_coords));
					indices.push_back(indices.size());
				}
			}
		}		
		file.close();

		cur_mesh.indices = indices;
		meshes.push_back(cur_mesh);		
		for (Mesh& mesh : meshes) {
			mesh.setup_mesh();
			if (!tex_path.empty())
				mesh.setup_texture(tex_path);
		}
	}
};
#endif