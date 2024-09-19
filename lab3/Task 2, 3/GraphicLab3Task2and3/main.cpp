#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

ImColor GetGradientColor(ImVec4& c1, ImVec4& c2, int segments, int segn) {
    ImColor res;
    res.Value.x = c1.x + (c2.x - c1.x) * segn / segments;
    res.Value.y = c1.y + (c2.y - c1.y) * segn / segments;
    res.Value.z = c1.z + (c2.z - c1.z) * segn / segments;
    res.Value.w = c1.w + (c2.w - c1.w) * segn / segments;
    return res;
}

void DrawPoint(int x, int y, float intencity, ImColor c) {
    c.Value.w *= intencity;
    ImGui::GetForegroundDrawList()->
        AddRectFilled(ImVec2(x, y), ImVec2(x + 1, y + 1), c);
}

void DrawLineBresenham(int x0, int y0, int x1, int y1, ImVec4 c1, ImVec4 c2) {
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
            DrawPoint(x0, y0, 1, GetGradientColor(c1, c2, segments, segn++));
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
            DrawPoint(x0, y0, 1, GetGradientColor(c1, c2, segments, segn++));
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

void DrawLineWu(int x0, int y0, int x1, int y1, ImVec4 c1, ImVec4 c2) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    float gradient = (float)dy / dx;
    int segn = 1;
    if (abs(gradient) <= 1) {
        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
            std::swap(c1, c2);
        }
        DrawPoint(x0, y0, 1, c1);
        float y = y0 + gradient;
        for (int x = x0 + 1; x <= x1 - 1; ++x) {
            ImColor c = GetGradientColor(c1, c2, x1 - x0 + 1, segn++);
            DrawPoint(x, (int)y, 1 - (y - (int)y), c);
            DrawPoint(x, (int)y + 1, y - (int)y, c);
            y += gradient;
        }
        DrawPoint(x1, y1, 1, c2);
    }
    else {
        gradient = (float)dx / dy;
        if (y0 > y1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
            std::swap(c1, c2);
        }
        DrawPoint(x0, y0, 1, c1);
        float x = x0 + gradient;
        for (int y = y0 + 1; y <= y1 - 1; ++y) {
            ImColor c = GetGradientColor(c1, c2, y1 - y0 + 1, segn++);
            DrawPoint((int)x, y, 1 - (x - (int)x), c);
            DrawPoint((int)x + 1, y, x - (int)x, c);
            x += gradient;
        }
        DrawPoint(x1, y1, 1, c2);
    }
}

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(800, 600, "Task 2", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(200, 200));
        ImGui::Begin("Line Parameters", NULL,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize);
        static int x0 = 0, y0 = 0, x1 = 800, y1 = 600;
        static ImVec4 color1 = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        static ImVec4 color2 = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        static bool IsBresenhamLine = true;
        static bool IsWuLine = false;

        ImGuiViewport* viewport = ImGui::GetMainViewport();


        //x0 = std::min(x0, (int)viewport->Size.x - 1);
        //y0 = std::min(y0, (int)viewport->Size.y - 1);
        //x1 = std::min(x1, (int)viewport->Size.x - 1);
        //y1 = std::min(y1, (int)viewport->Size.y - 1);

        ImGui::SliderInt("X0", &x0, 0, viewport->Size.x - 1);
        ImGui::SliderInt("Y0", &y0, 0, viewport->Size.y - 1);
        ImGui::ColorEdit4("Color1", (float*)&color1);
        ImGui::SliderInt("X1", &x1, 0, viewport->Size.x - 1);
        ImGui::SliderInt("Y1", &y1, 0, viewport->Size.y - 1);
        ImGui::ColorEdit4("Color2", (float*)&color2);
        if (ImGui::Checkbox("Bresenham", &IsBresenhamLine))
            IsWuLine = false;
        ImGui::SameLine();
        if (ImGui::Checkbox("Wu", &IsWuLine))
            IsBresenhamLine = false;
        ImGui::End();

        if (IsBresenhamLine)
            DrawLineBresenham(x0, viewport->Size.y - (y0 + 1),
                x1, viewport->Size.y - (y1 + 1),
                color1, color2);
        else
            DrawLineWu(x0, viewport->Size.y - (y0 + 1),
                x1, viewport->Size.y - (y1 + 1),
                color1, color2);

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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