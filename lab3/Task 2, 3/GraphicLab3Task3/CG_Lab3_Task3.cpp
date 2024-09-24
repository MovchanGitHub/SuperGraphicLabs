#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

struct Vertex {
    ImVec2 pos;
    ImVec4 color;
};

typedef void (*LineDrawer) (int x0, int y0, int x1, int y1, ImVec4 c1, ImVec4 c2);

ImColor GetGradientColor(const ImVec4& c1, const ImVec4& c2, float t) {
    ImColor res;
    res.Value.x = c1.x + (c2.x - c1.x) * t;
    res.Value.y = c1.y + (c2.y - c1.y) * t;
    res.Value.z = c1.z + (c2.z - c1.z) * t;
    res.Value.w = c1.w + (c2.w - c1.w) * t;
    return res;
}

void DrawPoint(int x, int y, float intensity, ImColor c) {
    c.Value.w *= intensity;
    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + 1, y + 1), c);
}

void DrawLineBresenham(int x0, int y0, int x1, int y1, ImVec4 c1, ImVec4 c2) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int xd = (x0 < x1) ? +1 : -1;
    int yd = (y0 < y1) ? +1 : -1;
    float gradient = (float)dy / dx;

    if (abs(gradient) <= 1) {
        int segments = abs(x1 - x0);
        int d = 2 * dy - dx;
        for (int i = 0; i <= segments; ++i) {
            float t = (float)i / segments;
            DrawPoint(x0, y0, 1, GetGradientColor(c1, c2, t));
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
            float t = (float)i / segments;
            DrawPoint(x0, y0, 1, GetGradientColor(c1, c2, t));
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
    if (abs(gradient) <= 1) {
        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
            std::swap(c1, c2);
        }
        DrawPoint(x0, y0, 1, c1);
        float y = y0 + gradient;
        for (int x = x0 + 1; x <= x1 - 1; ++x) {
            float t = (float)(x - x0) / (x1 - x0);
            ImColor c = GetGradientColor(c1, c2, t);
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
            float t = (float)(y - y0) / (y1 - y0);
            ImColor c = GetGradientColor(c1, c2, t);
            DrawPoint((int)x, y, 1 - (x - (int)x), c);
            DrawPoint((int)x + 1, y, x - (int)x, c);
            x += gradient;
        }
        DrawPoint(x1, y1, 1, c2);
    }
}

Vertex InterpolateVertex(float y, const Vertex& v1, const Vertex& v2) {
    float t = (y - v1.pos.y) / (v2.pos.y - v1.pos.y);
    return Vertex{ ImVec2(v1.pos.x + t * (v2.pos.x - v1.pos.x), y), GetGradientColor(v1.color, v2.color, t) };
}

void DrawGradientTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3, LineDrawer DrawLine) {
    const Vertex* vertices[3] = { &v1, &v2, &v3 };
    if (vertices[1]->pos.y < vertices[0]->pos.y) std::swap(vertices[0], vertices[1]);
    if (vertices[2]->pos.y < vertices[0]->pos.y) std::swap(vertices[0], vertices[2]);
    if (vertices[2]->pos.y < vertices[1]->pos.y) std::swap(vertices[1], vertices[2]);

    const Vertex& top = *vertices[0];
    const Vertex& mid = *vertices[1];
    const Vertex& bot = *vertices[2];

    for (float y = top.pos.y; y < mid.pos.y; ++y) {
        Vertex left = InterpolateVertex(y, top, mid);
        Vertex right = InterpolateVertex(y, top, bot);

        DrawLine(left.pos.x, left.pos.y, right.pos.x, right.pos.y, left.color, right.color);
    }

    for (float y = mid.pos.y; y < bot.pos.y; ++y) {
        Vertex left = InterpolateVertex(y, mid, bot);
        Vertex right = InterpolateVertex(y, top, bot);

        DrawLine(left.pos.x, left.pos.y, right.pos.x, right.pos.y, left.color, right.color);
    }
}


int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Task 2", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowSizeLimits(window, 1280, 720, 1280, 720);

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
        ImGui::SetNextWindowSize(ImVec2(1280, 720));
        ImGui::Begin("Triangle Parameters", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        static Vertex v1 = { ImVec2(250, 400), ImVec4(1.0f, 0.0f, 0.0f, 1.0f) };
        static Vertex v2 = { ImVec2(100, 600), ImVec4(0.0f, 1.0f, 0.0f, 1.0f) };
        static Vertex v3 = { ImVec2(400, 600), ImVec4(0.0f, 0.0f, 1.0f, 1.0f) };

        static std::vector<LineDrawer> LineDrawers = { DrawLineBresenham, DrawLineWu };

        static bool IsBresenhamLine = true;
        static bool IsWuLine = false;

        ImGui::SliderFloat("V1 X", &v1.pos.x, 0, 1280);
        ImGui::SliderFloat("V1 Y", &v1.pos.y, 250, 720);
        ImGui::ColorEdit4("V1 Color", (float*)&v1.color);
        ImGui::SliderFloat("V2 X", &v2.pos.x, 0, 1280);
        ImGui::SliderFloat("V2 Y", &v2.pos.y, 250, 720);
        ImGui::ColorEdit4("V2 Color", (float*)&v2.color);
        ImGui::SliderFloat("V3 X", &v3.pos.x, 0, 1280);
        ImGui::SliderFloat("V3 Y", &v3.pos.y, 250, 720);
        ImGui::ColorEdit4("V3 Color", (float*)&v3.color);

        if (ImGui::Checkbox("Bresenham", &IsBresenhamLine))
            IsWuLine = false;
        ImGui::SameLine();
        if (ImGui::Checkbox("Wu", &IsWuLine))
            IsBresenhamLine = false;
        ImGui::End();

        if (IsBresenhamLine)
            DrawGradientTriangle(v1, v2, v3, LineDrawers[0]);
        else
            DrawGradientTriangle(v1, v2, v3, LineDrawers[1]);

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