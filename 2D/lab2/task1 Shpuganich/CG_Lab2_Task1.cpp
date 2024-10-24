#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cmath>
#include <string>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const char* path_to_file = "image.jpg";
const int start_window_width = 1600;
const int start_window_height = 900;

typedef unsigned char (*transfer_func) (unsigned char r, unsigned char g, unsigned char b);

unsigned char func1(unsigned char r, unsigned char g, unsigned char b)
{
    return 0.299f * r + 0.587f * g + 0.114f * b;
}

unsigned char func2(unsigned char r, unsigned char g, unsigned char b)
{
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}


unsigned char* load_image_data(const char* filepath, int* img_width, int* img_height)
{
    int channels;
    unsigned char* data = stbi_load(filepath, img_width, img_height, &channels, 3);
    return data;
}

GLuint create_texture(unsigned char* img_data, int img_width, int img_height)
{
    GLuint image_texture;

    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB, GL_UNSIGNED_BYTE, img_data);

    return image_texture;
}


unsigned char* transfer_rgb_to_gray(unsigned char* img_data, int img_width, int img_height, transfer_func transfer_to_gray)
{
    unsigned char* gray_image_data = new unsigned char[img_width * img_height * 3];

    for (int i = 0; i < img_width * img_height; i++)
    {
        unsigned char red = img_data[i * 3];
        unsigned char green = img_data[i * 3 + 1];
        unsigned char blue = img_data[i * 3 + 2];

        unsigned char gray = transfer_to_gray(red, green, blue);

        for (int j = 0; j < 3; j++)
        {
            gray_image_data[i * 3 + j] = gray;
        }
    }

    return gray_image_data;
}


unsigned char* create_diff_image(unsigned char* gray_image_data_1, unsigned char* gray_image_data_2, int image_width, int image_height)
{
    unsigned char* diff_image_data = new unsigned char[image_width * image_height * 3];

    for (int i = 0; i < image_width * image_height; i++)
    {
        unsigned char diff_1 = gray_image_data_1[i * 3];
        unsigned char diff_2 = gray_image_data_2[i * 3];

        unsigned char diff = abs(diff_1 - diff_2);

        for (int j = 0; j < 3; j++)
        {
            diff_image_data[i * 3 + j] = diff;
        }
    }

    return diff_image_data;
}


void build_hist(const unsigned char* gray_image_data, int image_width, int image_height, float* histogram_float) {
    int histogram[256] = { 0 };

    for (int i = 0; i < image_width * image_height; i++) {
        unsigned char gray = gray_image_data[i * 3];
        histogram[gray]++;
    }

    for (int i = 0; i < 256; i++) {
        histogram_float[i] = static_cast<float>(histogram[i]);
    }
}

void draw_hists(const unsigned char* gray_image_data_1, const unsigned char* gray_image_data_2, int image_width, int image_height, int hist_width, int hist_height) {
    float histogram_float_1[256];
    float histogram_float_2[256];

    build_hist(gray_image_data_1, image_width, image_height, histogram_float_1);
    build_hist(gray_image_data_2, image_width, image_height, histogram_float_2);

    ImGui::SetNextItemWidth(hist_width);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PlotHistogram("##Nothing", histogram_float_1, 256, 0, nullptr, 0.0f, *std::max_element(histogram_float_1, histogram_float_1 + 256), ImVec2(0, hist_height));
    ImGui::PopStyleColor();

    ImGui::SetNextItemWidth(hist_width);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImGui::PlotHistogram("##Nothing", histogram_float_2, 256, 0, nullptr, 0.0f, *std::max_element(histogram_float_2, histogram_float_2 + 256), ImVec2(0, hist_height));
    ImGui::PopStyleColor();
}

int main(int, char**)
{
    if (!glfwInit())
        return 1;

    const char* glsl_ver = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(start_window_width, start_window_height, "LAB1", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_ver);

    int start_img_width;
    int start_img_height;

    unsigned char* og_image_data = load_image_data(path_to_file, &start_img_width, &start_img_height);
    if (!og_image_data) {
        return 0;
    }
    GLuint og_image_texture = create_texture(og_image_data, start_img_width, start_img_height);

    unsigned char* gray_image_data_1 = transfer_rgb_to_gray(og_image_data, start_img_width, start_img_height, func1);
    GLuint gray_image_texture_1 = create_texture(gray_image_data_1, start_img_width, start_img_height);

    unsigned char* gray_image_data_2 = transfer_rgb_to_gray(og_image_data, start_img_width, start_img_height, func2);
    GLuint gray_image_texture_2 = create_texture(gray_image_data_2, start_img_width, start_img_height);

    unsigned char* diff_image_data = create_diff_image(gray_image_data_1, gray_image_data_2, start_img_width, start_img_height);
    GLuint diff_image_texture = create_texture(diff_image_data, start_img_width, start_img_height);


    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            int window_width = io.DisplaySize.x;
            int window_height = io.DisplaySize.y;
            int img_width = window_width / 4;
            int img_height = window_height / 3;

            ImGui::SetNextWindowSize(ImVec2(window_width, window_height));
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::Begin("LAB2", nullptr, ImGuiWindowFlags_NoResize && ImGuiWindowFlags_NoCollapse);


            ImGui::Image((void*)(intptr_t)og_image_texture, ImVec2(img_width, img_height));

            ImGui::SameLine();
            ImGui::Image((void*)(intptr_t)gray_image_texture_1, ImVec2(img_width, img_height));

            ImGui::SameLine();
            ImGui::Image((void*)(intptr_t)gray_image_texture_2, ImVec2(img_width, img_height));

            ImGui::SameLine();
            ImGui::Image((void*)(intptr_t)diff_image_texture, ImVec2(img_width, img_height));


            draw_hists(gray_image_data_1, gray_image_data_2, start_img_width, start_img_height, window_width - 5, img_height - 12);

            ImGui::End();
        }


        ImGui::Render();
        int display_width, display_height;
        glfwGetFramebufferSize(window, &display_width, &display_height);
        glViewport(0, 0, display_width, display_height);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
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