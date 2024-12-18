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
#include <glm/gtc/matrix_transform.hpp>

const int NUM_PLANETS = 9;
const float ORBIT_RADIUS[] = { 0.0f, 3.0f, 5.0f, 7.0f, 10.0f, 15.0f, 20.0f, 25.0f, 30.0f };
const float PLANET_SIZE[] = { 3.0f, 1.5f, 1.3f, 1.2f, 1.1f, 1.0f, 0.9f, 0.8f, 0.7f };
// ?????? ??????
const float ORBIT_SPEED[] = { 0.0f, 0.02f, 0.04f, 0.06f, 0.08f, 0.10f, 0.12f, 0.14f, 0.16f };
// ?????? ????? ???
const float ROTATION_SPEED[] = { 0.0f, 0.2f, 0.25f, 0.3f, 0.35f, 0.4f, 0.45f, 0.5f, 0.6f };

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 tex_coords;

	Vertex(float pos_x, float pos_y, float pos_z) :
		position(glm::vec3(pos_x, pos_y, pos_z)) {}
};

struct Texture {
	GLuint id = -1;
	sf::Texture texture;
};

glm::vec3 translations[NUM_PLANETS];

inline void InitTranslations() {
	for (int i = 0; i < NUM_PLANETS; ++i) {
		translations[i] = glm::vec3(ORBIT_RADIUS[i], 0.0f, 0.0f);
	}
}

inline void CalculateOrbitTransform(glm::mat4* orbit_transform) {
	static float time = 0.0f;
	time += 0.01f;

	for (int i = 0; i < NUM_PLANETS; ++i) {
		auto& curr = orbit_transform[i];
		curr = glm::mat4(1.0f);

		float angle = ORBIT_SPEED[i] * time;
		translations[i] = glm::vec3(cos(angle) * ORBIT_RADIUS[i], 0.0f, sin(angle) * ORBIT_RADIUS[i]);

		curr = glm::translate(curr, translations[i]);
		curr = glm::rotate(curr, time * ROTATION_SPEED[i], glm::vec3(0.0f, 1.0f, 0.0f));
		curr = glm::scale(curr, glm::vec3(PLANET_SIZE[i]));
	}
}

class Mesh {
	void setup_mesh() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		//glGenBuffers(1, &instanceVBO);

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
		//glDeleteBuffers(1, &instanceVBO);
		glDeleteVertexArrays(1, &VAO);
	}

public:
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	Texture texture;
	GLuint VAO, VBO, EBO, instanceVBO;

	Mesh() = default;

	friend class ModelData;
	friend class Model;

	void display_mesh(GLuint shader_id) const {
		//static glm::mat4 orbit_transform[NUM_PLANETS];
		if (texture.id != -1) {
			glActiveTexture(GL_TEXTURE0);
			glUniform1i(glGetUniformLocation(shader_id, "material.texture"), 0);
			sf::Texture::bind(&texture.texture);
		}
		glBindVertexArray(VAO);

		//CalculateOrbitTransform(orbit_transform);
		//glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * NUM_PLANETS, orbit_transform, GL_STATIC_DRAW);
		//
		//for (int i = 0; i < 4; i++) {
		//	glEnableVertexAttribArray(3 + i);
		//	glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
		//	glVertexAttribDivisor(3 + i, 1);
		//}

		//glBindBuffer(GL_ARRAY_BUFFER, 0);

		//glDrawElementsInstanced(GL_TRIANGLES, (GLuint)indices.size(), GL_UNSIGNED_INT, 0, NUM_PLANETS);
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

struct ModelData {
	std::vector<Mesh> meshes;

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

	void release() {
		for (Mesh& mesh : meshes)
			mesh.release();
	}
};

struct Model {
	ModelData data;

	Model() = default;

	Model(std::string const& file_path, std::string const& tex_path) {
		data.load_model(file_path, tex_path);
	}

	void display_model(GLuint shader_id) {
		for (const Mesh& mesh : data.meshes)
			mesh.display_mesh(shader_id);
	}

	void release() {
		for (Mesh& mesh : data.meshes)
			mesh.release();
	}
};
#endif