#include <iostream>
#include <vector>
#include <list>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define _USE_MATH_DEFINES
#include <math.h>

class point {
    std::vector<double> cords;

public:
    double& x;
    double& y;
    point(double x, double y) : cords{ x, y, 1 }, x(cords[0]), y(cords[1]) {}
    point(const point& p) : cords(p.cords), x(cords[0]), y(cords[1]) {}
    point& operator=(const point& p) {
        cords[0] = p.cords[0];
        cords[1] = p.cords[1];
        return *this;
    }
    void affine_transformation(std::vector<std::vector<double>>& m) {
        std::vector<double> ncords(3);
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                ncords[j] += cords[k] * m[k][j];
        cords = ncords;
    }
};

bool operator==(const point& p1, const point& p2) {
    return p1.x == p2.x && p1.y == p2.y;
}

bool operator!=(const point& p1, const point& p2) {
    return p1.x != p2.x || p1.y != p2.y;
}

ImColor GetGradientColor(ImVec4& c1, ImVec4& c2, int segments, int segn) {
    ImColor res;
    res.Value.x = c1.x + (c2.x - c1.x) * segn / segments;
    res.Value.y = c1.y + (c2.y - c1.y) * segn / segments;
    res.Value.z = c1.z + (c2.z - c1.z) * segn / segments;
    res.Value.w = c1.w + (c2.w - c1.w) * segn / segments;
    return res;
}

void DrawPoint(int x, int y, float intencity, ImColor c, int offset = 0) {
    c.Value.w *= intencity;
    ImGui::GetForegroundDrawList()->
        AddRectFilled(ImVec2(x - offset, y - offset), ImVec2(x + 1 + offset, y + 1 + offset), c);
}

void DrawLineWu(point p0, point p1, ImVec4 c1, ImVec4 c2) {
    int x0 = p0.x;
    int y0 = p0.y;
    int x1 = p1.x;
    int y1 = p1.y;

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

void DrawBoldPoint(int x, int y, ImVec4 color)
{
    for (int i = -2; i <= 2; i++)
    {
        for (int j = -2; j <= 2; j++)
        {
            DrawPoint(x + i, y + j, 1, color);
        }
    }
}

std::vector<point> points;
std::vector<point>::iterator catched_point;
bool isDragMode = false;
bool isPointCathced = false;

void DrawListOfPoints()
{
    for (auto point : points)
    {
        DrawBoldPoint(point.x, point.y, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    }
}

void DeletePoint(int x, int y)
{
    for (auto it = points.begin(); it != points.end() ; ++it)
    {
        if (abs((*it).x - x) < 6 && abs((*it).y - y) < 6)
        {
            points.erase(it);
            return;
        }
    }
}

void CatchPoint(int x, int y)
{
    for (auto it = points.begin(); it != points.end(); ++it)
    {
        if (abs((*it).x - x) < 6 && abs((*it).y - y) < 6)
        {
            catched_point = it;
            isPointCathced = true;
            return;
        }
    }
}

void DropPoint()
{
    catched_point = points.end();
    isPointCathced = false;
}

void DrawBezierCurve(point p0, point p1, point p2, point p3)
{
    std::vector<point> pv = { p0, p1, p2, p3 };
    std::vector<std::vector<int>> matr = { {1, -3, 3, -1}, {0, 3, -6, 3}, {0, 0, 3, -3}, {0, 0, 0, 1} };
    std::vector<float> t = { 0, 0, 0, 0 };

    std::vector<point> m = { {0, 0}, {0, 0}, {0, 0}, {0, 0} };
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            m[i].x += pv[j].x * matr[j][i];
            m[i].y += pv[j].y * matr[j][i];
        }
    }

    for (float i = 0; i <= 1; i += 0.0001)
    {
        t[0] = 1;
        t[1] = i;
        t[2] = i * i;
        t[3] = t[2] * i;
        point p = { 0, 0 };
        for (int j = 0; j < 4; ++j)
        {
            p.x += m[j].x * t[j];
            p.y += m[j].y * t[j];
        }
        DrawPoint(p.x, p.y, 1, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    }
}

void DrawBezierCurve(point p0, point p1, point p2)
{
    point p23 = { p0.x + 2 * (p1.x - p0.x) / 3, p0.y + 2 * (p1.y - p0.y) / 3 };
    point p13 = { p1.x + (p2.x - p1.x) / 3, p1.y + (p2.y - p1.y) / 3 };
    DrawBezierCurve(p0, p23, p13, p2);
}

void DrawCompositeBeizerCurve()
{
    if (points.size() < 2)
    {
        return;
    }

    if (points.size() == 2)
    {
        DrawLineWu(*points.begin(), *(std::next(points.begin(), 1)), ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        return;
    }

    point prev = points[0];
    int points_size = (int)points.size();
    for (int i = 0; i < points_size - 4; i += 2)
    {
        point next = { (points[i + 2].x + points[i + 3].x) / 2, (points[i + 2].y + points[i + 3].y) / 2 };
        DrawBezierCurve(prev, points[i + 1], points[i + 2], next);
        prev = next;
    }

    if (points.size() % 2 == 0)
    {
        DrawBezierCurve(prev, points[points_size - 3], points[points_size - 2], points[points_size - 1]);
    }
    else
    {
        DrawBezierCurve(prev, points[points_size - 2], points[points_size - 1]);
    }
}

void draw_UI() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 102));
    ImGui::Begin("Instruments", NULL,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse);

    if (ImGui::Button("DragMode", ImVec2(100, 50)) && !isPointCathced) {
        isDragMode = !isDragMode;
    }

    ImGui::End();
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < IM_ARRAYSIZE(io.MouseDown)) {
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
    }

    if (io.WantCaptureMouse) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if (!isDragMode)
        {
            points.push_back({ xpos, ypos });
            return;
        }
        if (!isPointCathced)
        {
            CatchPoint(xpos, ypos);
            return;
        }
        if (isPointCathced)
        {
            DropPoint();
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        if (!isDragMode)
        {
            DeletePoint(xpos, ypos);
        }
    }
}

int main() {
    setlocale(LC_ALL, "russian");
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(1600, 800, "Lab4", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwSetWindowSizeLimits(window, 1600, 800, 1800, 800);
    glfwMakeContextCurrent(window);
    glewInit();
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    point mouse_point = point(0, 0);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_UI();

        DrawListOfPoints();
        DrawCompositeBeizerCurve();

        if (isDragMode)
        {
            glfwGetCursorPos(window, &mouse_point.x, &mouse_point.y);
            if (isPointCathced)
            {
                (*catched_point).x = mouse_point.x;
                (*catched_point).y = mouse_point.y;
            }
        }

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