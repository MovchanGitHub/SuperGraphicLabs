#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#ifdef	_WIN32
#include <GL/wglew.h>
#else
#include <GL/glxew.h>
#endif
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <set>

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

struct Line {
	ImVec2 start;
	ImVec2 end;
};

struct image_info {
	int width, height, channels;
	GLuint texture;
	std::vector<unsigned char> data;
	bool is_success = 1;
};

struct rgb {
	unsigned char r, g, b;
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

image_info load_white_texture(const ImVec2& size) {
	image_info res{ size.x, size.y, 3, 0, std::vector<unsigned char>(size.x * size.y * 3, 255) };

	glGenTextures(1, &res.texture);
	glBindTexture(GL_TEXTURE_2D, res.texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, res.data.data());

	return res;
}

ImVec2 img_size(const image_info& img, int size_x, int size_y) {
	float res_y = size_y - 100;
	float res_x = img.width * res_y / img.height;
	if (res_x > size_x) {
		res_x = size_x;
		res_y = img.height * res_x / img.width;
	}
	return ImVec2{ res_x, res_y };
}

void flood_fill(std::vector<unsigned char>& image_data, int w, int h, int x, int y, const rgb& new_color, const rgb& old_color) {
	int pix_ind = (x + y * w) * 3;
	if (x > 0 && x < w && y >= 0 && y < h &&
		(image_data[pix_ind] == old_color.r && image_data[pix_ind + 1] == old_color.g && image_data[pix_ind + 2] == old_color.b) &&
		(image_data[pix_ind] != new_color.r && image_data[pix_ind + 1] != new_color.g && image_data[pix_ind + 2] != new_color.b)) {
		int x_left = x;
		while (x_left >= 0 && (image_data[pix_ind] == old_color.r && image_data[pix_ind + 1] == old_color.g && image_data[pix_ind + 2] == old_color.b)) {
			x_left--;
			pix_ind = (x_left + y * w) * 3;
		}
		x_left++;

		int x_right = x;
		pix_ind = (x + y * w) * 3;
		while (x_right && (image_data[pix_ind] == old_color.r && image_data[pix_ind + 1] == old_color.g && image_data[pix_ind + 2] == old_color.b)) {
			x_right++;
			pix_ind = (x_right + y * w) * 3;
		}
		x_right--;

		for (int i = x_left; i < x_right + 1; ++i) {
			pix_ind = (i + y * w) * 3;
			image_data[pix_ind] = new_color.r;
			image_data[pix_ind + 1] = new_color.g;
			image_data[pix_ind + 2] = new_color.b;
		}

		for (int i = x_left; i < x_right + 1; ++i) {
			flood_fill(image_data, w, h, i, y - 1, new_color, old_color);
			flood_fill(image_data, w, h, i, y + 1, new_color, old_color);
		}
	}
}

void flood_fill_pic1(std::vector<unsigned char>& image_data, int w, int h, int x, int y, const rgb& old_color, const std::vector<unsigned char>& fill_image_data, int fill_w, int fill_h, int fill_x, int fill_y) {
	int pix_ind = (x + y * w) * 3;
	if (x > 0 && x < w && y >= 0 && y < h &&
		(image_data[pix_ind] == old_color.r && image_data[pix_ind + 1] == old_color.g && image_data[pix_ind + 2] == old_color.b)) {
		int x_left = x;
		while (x_left >= 0 && (image_data[pix_ind] == old_color.r && image_data[pix_ind + 1] == old_color.g && image_data[pix_ind + 2] == old_color.b)) {
			x_left--;
			pix_ind = (x_left + y * w) * 3;
		}
		x_left++;

		int x_right = x;
		pix_ind = (x + y * w) * 3;
		while (x_right < w && (image_data[pix_ind] == old_color.r && image_data[pix_ind + 1] == old_color.g && image_data[pix_ind + 2] == old_color.b)) {
			x_right++;
			pix_ind = (x_right + y * w) * 3;
		}
		x_right--;
		for (int i1 = x, i2 = fill_x; i1 > x_left - 1; --i1, --i2) {
			if (i2 == -1)
				i2 = fill_w - 1;
			pix_ind = (i1 + y * w) * 3;
			int pix_ind2 = (i2 + fill_y * fill_w) * 3;
			image_data[pix_ind] = fill_image_data[pix_ind2];
			image_data[pix_ind + 1] = fill_image_data[pix_ind2 + 1];
			image_data[pix_ind + 2] = fill_image_data[pix_ind2 + 2];
		}
		for (int i1 = x, i2 = fill_x - fill_w + 1; i1 < x_right + 1; ++i1, ++i2) {
			if (i2 >= fill_w)
				i2 = 0;
			pix_ind = (i1 + y * w) * 3;
			int pix_ind2 = (i2 + fill_y * fill_w) * 3;
			image_data[pix_ind] = fill_image_data[pix_ind2];
			image_data[pix_ind + 1] = fill_image_data[pix_ind2 + 1];
			image_data[pix_ind + 2] = fill_image_data[pix_ind2 + 2];
		}



		for (int i = x; i < x_right + 1; ++i) {
			flood_fill_pic1(image_data, w, h, i, y - 1, old_color, fill_image_data, fill_w, fill_h, (fill_x + i >= w) ? i : fill_x + i, (fill_y > 0) ? fill_y - 1 : fill_h - 1);
			flood_fill_pic1(image_data, w, h, i, y + 1, old_color, fill_image_data, fill_w, fill_h, (fill_x + i >= w) ? i : fill_x + i, (fill_y < fill_h - 1) ? fill_y + 1 : 0);
		}
	}
}

struct Compare {
	bool operator()(std::pair<int, int>* a, std::pair<int, int>* b) const {
		if (a->second == b->second) {
			return a->first < b->first;
		}
		return a->second < b->second;
	}
};

void border_selection(std::vector<unsigned char>& image_data, int w, int h, int start_x, int start_y, const rgb& border_color) {
	if (!(start_x > 0 && start_x < w && start_y >= 0 && start_y < h))
		return;
	int x_border = start_x, pix_ind = (start_x + start_y * w) * 3;
	while (x_border < w && (image_data[pix_ind] != border_color.r && image_data[pix_ind + 1] != border_color.g && image_data[pix_ind + 2] != border_color.b)) {
		x_border++;
		pix_ind = (x_border + start_y * w) * 3;
	}
	//x_right--;
	if (x_border == w)
	{
		x_border = start_x;
		pix_ind = (start_x + start_y * w) * 3;
		while (x_border >= 0 && (image_data[pix_ind] != border_color.r && image_data[pix_ind + 1] != border_color.g && image_data[pix_ind + 2] != border_color.b)) {
			x_border--;
			pix_ind = (x_border + start_y * w) * 3;
		}
	}
	if (x_border == -1)
		return;
	//x_left++;
	std::multiset<std::pair<int, int>*, Compare> points;
	std::pair<int, int>* start = new std::pair<int, int>(x_border, start_y);
	std::pair<int, int>* cur = new std::pair<int, int>(x_border, start_y);
	points.insert(start);
	int cur_dir = 6, next_dir;
	while (1) {
		switch (cur_dir) {
		case 0:
			cur->first++;
			break;
		case 1:
			cur->first++;
			cur->second--;
			break;
		case 2:
			cur->second--;
			break;
		case 3:
			cur->first--;
			cur->second--;
			break;
		case 4:
			cur->first--;
			break;
		case 5:
			cur->first--;
			cur->second++;
			break;
		case 6:
			cur->second++;
			break;
		case 7:
			cur->first++;
			cur->second++;
			break;
		}
		if (cur->first == start->first && cur->second == start->second)
			break;
		pix_ind = (cur->first + cur->second * w) * 3;
		if (image_data[pix_ind] == border_color.r && image_data[pix_ind + 1] == border_color.g && image_data[pix_ind + 2] == border_color.b) {
			points.insert(new std::pair<int, int>{ cur->first, cur->second });
			cur_dir = (cur_dir + 6) % 8;
		}
		else
			cur_dir = (cur_dir + 4) % 8;
	}
	for (auto& point : points) {
		int pix_ind = (point->first + point->second * w) * 3;
		image_data[pix_ind] = 255;
		image_data[pix_ind + 1] = 0;
		image_data[pix_ind + 2] = 0;
	}
}

void DrawLineBresenham(int x0, int y0, int x1, int y1, int w, ImColor c, image_info& img) {//std::vector<unsigned char>& img_data) {
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int xd = (x0 < x1) ? +1 : -1;
	int yd = (y0 < y1) ? +1 : -1;
	float gradient = (float)dy / dx;
	int segn = 0;
	if (abs(gradient) <= 1) {
		int segments = abs(x1 - x0);
		int d = 2 * dy - dx;
		for (int i = 0; i <= segments; ++i) {
			//DrawPoint(x0, y0, 1, c);
			int pix_ind = (x0 + y0 * w) * 3;
			img.data[pix_ind] = 0;
			img.data[pix_ind + 1] = 0;
			img.data[pix_ind + 2] = 0;
			if (d < 0)
				d += 2 * dy;
			else {
				y0 += yd;
				d += 2 * (dy - dx);
			}
			x0 += xd;
		}
	}
	else {
		int segments = abs(y1 - y0);
		int d = 2 * dx - dy;
		for (int i = 0; i <= segments; ++i) {
			//DrawPoint(x0, y0, 1, c);
			int pix_ind = (x0 + y0 * w) * 3;
			img.data[pix_ind] = 0;
			img.data[pix_ind + 1] = 0;
			img.data[pix_ind + 2] = 0;
			if (d < 0)
				d += 2 * dx;
			else {
				x0 += xd;
				d += 2 * (dx - dy);
			}
			y0 += yd;
		}
	}
}
/*
ImDrawData makeDrawData(ImDrawList** dl, ImVec2 pos, ImVec2 size) {
	ImDrawData draw_data = ImDrawData();

	draw_data.Valid = true;
	ImVector<ImDrawList*> weird_vector;
	weird_vector.
	draw_data.CmdLists = dl;
	draw_data.CmdListsCount = 1;
	draw_data.TotalVtxCount = (*dl)->VtxBuffer.size();
	draw_data.TotalIdxCount = (*dl)->IdxBuffer.size();
	draw_data.DisplayPos = pos;
	draw_data.DisplaySize = size;
	draw_data.FramebufferScale = ImVec2(1, 1);

	return draw_data;
}
*/
int main(int, char**) {
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return -1;

	const char* glsl_version = "#version 130";
	GLFWwindow* window = glfwCreateWindow(1600, 900, "Paint-like Drawing", NULL, NULL);
	if (window == NULL)
		return -1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	if (glewInit() != GLEW_OK)
		return -1;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	image_info background = load_white_texture(ImVec2{ 1500, 900 });
	image_info pic_for_fill = load_texture("something.jpg");
	ImVec2 last_pos(-1, -1);
	bool is_drawing = 0, draw_mode = 1, fill_mode_color = 0, fill_mode_pic = 0;
	std::vector<Line> lines;
	/*
	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, img.texture, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "Error: Framebuffer is not complete!\n");
		return -1;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	*/
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Start the ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize({ io.DisplaySize.x, io.DisplaySize.y });
		ImGui::Begin("Paint-like Drawing", (bool*)0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::Text("Use the mouse to draw.");
		if (ImGui::Button("Draw")) {
			draw_mode = 1;
			fill_mode_color = 0;
			fill_mode_pic = 0;
		}
		ImGui::SameLine();
		if (ImGui::Button("Fill with color")) {
			draw_mode = 0;
			fill_mode_color = 1;
			fill_mode_pic = 0;
		}
		ImGui::SameLine();
		if (ImGui::Button("Fill with picture")) {
			draw_mode = 0;
			fill_mode_color = 0;
			fill_mode_pic = 1;
		}
		ImVec2 cur_pos = ImGui::GetCursorPos();
		ImGui::InvisibleButton("canvas", ImVec2{ (float)background.width, (float)background.height });
		ImGui::SetNextItemAllowOverlap();
		ImGui::SetCursorScreenPos(cur_pos);
		if (ImGui::IsItemHovered()) {
			if (ImGui::IsMouseDown(0) && draw_mode) {
				if (!is_drawing) {
					is_drawing = true;
					last_pos = ImGui::GetMousePos();
				}
				ImVec2 current_pos = ImGui::GetMousePos();
				lines.push_back({ last_pos, current_pos });
				last_pos = current_pos;
			}
			else if (ImGui::IsMouseDown(0) && fill_mode_color) {
				ImVec2 current_pos = ImGui::GetMousePos();
				int pix_ind = (current_pos.x + current_pos.y * background.width) * background.channels;
				flood_fill(background.data, background.width, background.height, current_pos.x, current_pos.y, rgb{ 50, 0, 0 }, rgb{ background.data[pix_ind], background.data[pix_ind + 1], background.data[pix_ind + 2] });
			}
			else if (ImGui::IsMouseDown(0) && fill_mode_pic) {
				ImVec2 current_pos = ImGui::GetMousePos();
				int pix_ind = (current_pos.x + current_pos.y * background.width) * background.channels;
				flood_fill_pic1(background.data, background.width, background.height, current_pos.x, current_pos.y, rgb{ background.data[pix_ind], background.data[pix_ind + 1], background.data[pix_ind + 2] }, pic_for_fill.data, pic_for_fill.width, pic_for_fill.height, pic_for_fill.width - 1, pic_for_fill.height - 1);
				//border_selection(background.data, background.width, background.height, current_pos.x, current_pos.y, rgb{ 0, 0, 0 });
				flood_fill(background.data, background.width, background.height, current_pos.x, current_pos.y, rgb{ 50, 0, 0 }, rgb{ background.data[pix_ind], background.data[pix_ind + 1], background.data[pix_ind + 2] });
			}
			else {
				is_drawing = false;
			}

		}
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		for (const Line& line : lines) {
			//draw_list->AddLine(line.start, line.end, IM_COL32(0, 0, 255, 255), 2.0f);
			DrawLineBresenham(line.start.x, line.start.y, line.end.x, line.end.y, background.width, IM_COL32(0, 0, 255, 255), background);
		}

		glBindTexture(GL_TEXTURE_2D, background.texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, background.width, background.height, 0, GL_RGB, GL_UNSIGNED_BYTE, background.data.data());
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		ImGui::Image((void*)(intptr_t)background.texture, img_size(background, display_w, display_h));
		ImGui::End();

		ImGui::Render();
		glViewport(0, 0, display_w, display_h);
		//glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
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