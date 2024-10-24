#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

const int CHANNELRED = 0;
const int CHANNELGREEN = 1;
const int CHANNELBLUE = 2;
const float HISTHEIGHT = 120;
//const std::string pic_name = "p1.jpg";
//const std::string pic_name = "p2.jfif";
//const std::string pic_name = "p3.jpg";
const std::string pic_name = "p4.jpg";

struct ImageData {
    int width, height, channels;
    std::vector<unsigned char> data;
};

ImageData load_image(const char* filename) {
    ImageData img;
    unsigned char* data = stbi_load(filename, &img.width, &img.height, &img.channels, STBI_rgb);
    if (data) {
        img.data.assign(data, data + img.width * img.height * img.channels);
        stbi_image_free(data);
    }
    return img;
}

float histogram[3][256];
float histogram_max_value;

void calc_histograms(const std::vector<unsigned char>& data, int width, int height, int channels) {
    for (int i = 0; i < width * height * channels; i += channels) {
        histogram[CHANNELRED][data[i]]++;
        histogram[CHANNELGREEN][data[i + 1]]++;
        histogram[CHANNELBLUE][data[i + 2]]++;
    }

    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 256; ++j)
            if (histogram[i][j] > histogram_max_value)
                histogram_max_value = histogram[i][j];
}

std::vector<unsigned char> get_channel(const std::vector<unsigned char>& data, int width, int height, int channels, int channelIndex) {
    std::vector<unsigned char> channelData(width * height * 3, 0);

    for (int i = 0; i < width * height; i++)
        channelData[3 * i + channelIndex] = data[channels * i + channelIndex];

    return channelData;
}

GLuint create_texture(const std::vector<unsigned char>& data, int width, int height) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return textureID;
}

void text_centered(std::string text) {
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth = ImGui::CalcTextSize(text.c_str()).x;

    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::Text(text.c_str());
}

ImVec2 calc_image_size(const ImageData& img, float window_width, float window_height) {
    float image_width = window_width / 3;
    float image_height = image_width * img.height / img.width;
    if (image_height > window_height - 3 * (HISTHEIGHT + 26)) {
        image_height = window_height - 3 * (HISTHEIGHT + 26);
        image_width = image_height * img.width / img.height;
    }
    return ImVec2(image_width, image_height);
}

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "Lab2Task2", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImageData img = load_image(pic_name.c_str());
    if (img.data.empty()) {
        std::cout << "Fail to load image";
        return -1;
    }

    std::vector<unsigned char> redChannel = get_channel(img.data, img.width, img.height, img.channels, 0);
    std::vector<unsigned char> greenChannel = get_channel(img.data, img.width, img.height, img.channels, 1);
    std::vector<unsigned char> blueChannel = get_channel(img.data, img.width, img.height, img.channels, 2);

    calc_histograms(img.data, img.width, img.height, img.channels);

    GLuint redTexture = create_texture(redChannel, img.width, img.height);
    GLuint greenTexture = create_texture(greenChannel, img.width, img.height);
    GLuint blueTexture = create_texture(blueChannel, img.width, img.height);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGui::Begin("Image Channels", NULL,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        ImVec2 image_size = calc_image_size(img, viewport->Size.x, viewport->Size.y);

        ImGui::Image((void*)(intptr_t)redTexture, image_size);
        ImGui::SameLine();
        ImGui::Image((void*)(intptr_t)greenTexture, image_size);
        ImGui::SameLine();
        ImGui::Image((void*)(intptr_t)blueTexture, image_size);

        text_centered("Red Histogram");
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::PlotHistogram("Red", histogram[CHANNELRED], 256, 0, NULL, 0.0f, histogram_max_value, ImVec2(viewport->Size.x, HISTHEIGHT));
        ImGui::PopStyleColor();

        text_centered("Green Histogram");
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
        ImGui::PlotHistogram("Green", histogram[CHANNELGREEN], 256, 0, NULL, 0.0f, histogram_max_value, ImVec2(viewport->Size.x, HISTHEIGHT));
        ImGui::PopStyleColor();

        text_centered("Blue Histogram");
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
        ImGui::PlotHistogram("Blue", histogram[CHANNELBLUE], 256, 0, NULL, 0.0f, histogram_max_value, ImVec2(viewport->Size.x, HISTHEIGHT));
        ImGui::PopStyleColor();

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
