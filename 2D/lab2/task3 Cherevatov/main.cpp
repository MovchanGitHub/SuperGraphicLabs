#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <stdlib.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

struct image_info {
	int width, height, channels;
	GLuint texture;
	bool is_success;
	std::vector<unsigned char> data;
};

struct hsv {
	float hue, saturation, value;
};

image_info load_texture(const char* file_name) {
	image_info res;
	res.is_success = 0;
	unsigned char* image_data = stbi_load(file_name, &res.width, &res.height, &res.channels, 3);
	if (image_data == nullptr)
		return res;
	
	glGenTextures(1, &res.texture);
	glBindTexture(GL_TEXTURE_2D, res.texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, res.width, res.height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);

	res.data = std::vector<unsigned char>(res.width * res.height * res.channels);
	for (int i = 0; i < res.width * res.height * res.channels; i += res.channels)
		for (int j = 0; j < res.channels; ++j)
			res.data[i + j] = image_data[i + j];
	stbi_image_free(image_data);
	res.is_success = 1;

	return res;
}

ImVec2 img_size(image_info& img, int size_x, int size_y) {
	float res_y = size_y - 100;
	float res_x = img.width * res_y / img.height;
	if (res_x > size_x) {
		res_x = size_x;
		res_y = img.height * res_x / img.width;
	}
	return ImVec2{ res_x, res_y };
}

hsv rgb_to_hsv(const std::vector<float>& rgb) {
	hsv res{ 0, 0, 0 };
	float max_color = std::max(rgb[0], std::max(rgb[1], rgb[2]));
	float min_color = std::min(rgb[0], std::min(rgb[1], rgb[2]));
	res.value = max_color;
	if (std::abs(max_color) < 0.0001f)
		return res;
	res.saturation = 1 - min_color / max_color;
	if (std::abs(max_color - min_color) < 0.0001f)
		return res;
	float diff = max_color - min_color;
	if (std::abs(rgb[1] - max_color) < 0.0001f)
		res.hue = 60 * (rgb[2] - rgb[0]) / diff + 120;
	else if (std::abs(rgb[2] - max_color) < 0.0001f)
		res.hue = 60 * (rgb[0] - rgb[1]) / diff + 240;
	else {
		res.hue = 60 * (rgb[1] - rgb[2]) / diff;
		if (rgb[1] < rgb[2])
			res.hue += 360;
	}
	return res;
}

void hsv_to_rgb(std::vector<float>& rgb, std::vector<float>& hsv) {
	if (hsv[0] > 359.9f)
		hsv[0] = 359.9f;
	int h_i = (int)std::floor(hsv[0] / 60) % 6;
	float f = hsv[0] / 60 - std::floor(hsv[0] / 60), p = hsv[2] * (1 - hsv[1]);
	float q = hsv[2] * (1 - f * hsv[1]), t = hsv[2] * (1 - (1 - f) * hsv[1]);
	switch (h_i) {
	case 1:
		rgb[0] = q;
		rgb[1] = hsv[2];
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = hsv[2];
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = hsv[2];
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = hsv[2];
		break;
	case 5:
		rgb[0] = hsv[2];
		rgb[1] = p;
		rgb[2] = q;
		break;
	default:
		rgb[0] = hsv[2];
		rgb[1] = t;
		rgb[2] = p;
	}
}


int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return -1;

	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "CG lab 2", nullptr, nullptr);
	if (window == nullptr)
		return -1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	ImVec4 clear_color = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	image_info img = load_texture("img1.jpg");
	if (!img.is_success)
		return -1;
	
	std::vector<hsv> hsv_image;
	std::vector<unsigned char> res_image(img.width * img.height * img.channels);
	std::vector<float> rgb(3);
	std::vector<float> hsv_temp(3);
	std::vector<float> hsv_slider{ 0, 1, 1 };
	for (int i = 0; i < img.width * img.height * img.channels; i += img.channels) {
		for (int j = 0; j < img.channels; ++j)
			rgb[j] = img.data[i + j] / 255.0f;
		hsv_image.push_back(rgb_to_hsv(rgb));
	}	

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize({ io.DisplaySize.x, io.DisplaySize.y });	
		ImGui::Begin("Image to HSV", (bool*)0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::SliderFloat("Hue", &hsv_slider[0], 0, 359.9);
		ImGui::SliderFloat("Saturation", &hsv_slider[1], 0, 1);
		ImGui::SliderFloat("Value", &hsv_slider[2], 0, 1);
		ImGui::PopItemWidth();

		for (int i = 0; i < hsv_image.size(); ++i) {
			hsv_temp[0] = hsv_image[i].hue + hsv_slider[0];
			hsv_temp[1] = hsv_image[i].saturation * hsv_slider[1];
			hsv_temp[2] = hsv_image[i].value * hsv_slider[2];
			hsv_to_rgb(rgb, hsv_temp);
			for (int j = 0; j < 3; ++j)
				res_image[i * 3 + j] = static_cast<unsigned char>(rgb[j] * 255.0f);
		}

		glBindTexture(GL_TEXTURE_2D, img.texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.width, img.height, 0, GL_RGB, GL_UNSIGNED_BYTE, res_image.data());		
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);		
		ImGui::Image((void*)(intptr_t)img.texture, img_size(img, display_w, display_h));
		ImGui::End();

		ImGui::Render();
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());		
		glfwSwapBuffers(window);
	}
	stbi_write_png("result.jpg", img.width, img.height, 3, res_image.data(), img.width * 3);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}