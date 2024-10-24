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

typedef float fun_type(float);
typedef fun_type* point_fun_type;

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static float num_sin(float num) {
	return sin(num);
}

static float num_squared(float num) {
	return num * num;
}

static float num_arcsin(float num) {
	return asin(num);
}

static float cube_root(float num) {
	return cbrt(num);
}

static void draw_graph(ImDrawList* draw_list, const ImVec2& begin, const ImVec2& size, point_fun_type fun, float x_min, float x_max, int point_count) {
	std::vector<ImVec2> pts;
	float x = x_min;
	float inc = (x_max - x_min) / (point_count - 1);
	float y;
	float y_min = FLT_MAX;
	float y_max = FLT_MIN;
	for (int i = 0; i < point_count; i++) {
		y = fun(x);
		if (y < y_min)
			y_min = y;
		if (y > y_max)
			y_max = y;
		pts.push_back(ImVec2(x, y));
		x += inc;
	}
	for (int i = 0; i < point_count; i++) {
		pts[i].x = (pts[i].x - x_min) / (x_max - x_min) * size.x+ begin.x;
		pts[i].y = size.y - (pts[i].y - y_min) / (y_max - y_min) * size.y + begin.y;
	}
	float pos_x_axis = size.x * -x_min / (x_max - x_min);
	float pos_y_axis = size.y * y_max / (y_max - y_min);
	draw_list->AddQuad(begin, ImVec2(begin.x + size.x, begin.y), ImVec2(begin.x + size.x, begin.y + size.y), ImVec2(begin.x, begin.y + size.y), IM_COL32(0, 0, 0, 255));
	draw_list->AddLine(ImVec2(begin.x, begin.y + pos_y_axis), ImVec2(begin.x + size.x, begin.y + pos_y_axis), IM_COL32(0, 0, 0, 255));
	draw_list->AddLine(ImVec2(begin.x + pos_x_axis, begin.y), ImVec2(begin.x + pos_x_axis, begin.y + size.y), IM_COL32(0, 0, 0, 255));
	for (int i = 0; i < point_count - 1; i++)
		draw_list->AddLine(pts[i], pts[i + 1], IM_COL32(11, 138, 70, 255), 2.5f);
}


int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "CG lab 1", nullptr, nullptr);
	if (window == nullptr)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	ImVec4 clear_color = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsLight();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize({ io.DisplaySize.x, io.DisplaySize.y });
		ImGui::Begin("Graph", (bool*)0, ImGuiWindowFlags_NoResize);

		static int cur_fun = 0;
		std::vector<point_fun_type> fun_vec;
		fun_vec.push_back(num_sin);
		fun_vec.push_back(num_arcsin);
		fun_vec.push_back(cube_root);
		fun_vec.push_back(num_squared);
		const char* fun_names[] = { "sin(x)", "arcsin(x)", "x^(1/3)", "x^2" };
		ImGui::Combo("Choose a function", &cur_fun, fun_names, IM_ARRAYSIZE(fun_names));

		static float vals[2] = { -10.0f, 10.0f };
		static int point_count = 1000;
		ImGui::InputFloat2("Set min and max values", vals);
		ImGui::InputInt("Number of points", &point_count);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_graph(draw_list, ImGui::GetCursorScreenPos(), ImGui::GetContentRegionAvail(), fun_vec[cur_fun], vals[0], vals[1], point_count);
		ImGui::End();

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
