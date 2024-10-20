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

void DrawLineBresenham(point p0, point p1, ImVec4 c1, ImVec4 c2) {
    int x0 = p0.x;
    int y0 = p0.y;
    int x1 = p1.x;
    int y1 = p1.y;

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

std::list<point> points;
float r = 0.25;
int max_depth = 10, cur_depth = 0;
bool is_placing_point = 1, step_by_step_drawing = 0;
std::string text_str = "Place 2 points";

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < IM_ARRAYSIZE(io.MouseDown)) {
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
    }

    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (is_placing_point) {
            if (points.size() == 2) {
                (*std::prev(points.end())).x = xpos;
                (*std::prev(points.end())).y = ypos;
            }
            else
                points.push_back(point{ xpos, ypos });
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        is_placing_point = 1;
        step_by_step_drawing = 0;
        cur_depth = 0;
        text_str = "Place 2 points";
        points.clear();
    }
}

void midpoint_displacement(std::list<point>::iterator p1, std::list<point>::iterator p2, int depth = max_depth) {
    if (depth == 0)
        return;
    double h = ((*p1).y + (*p2).y) / 2;
    double len = std::sqrt(std::pow((*p2).x - (*p1).x, 2) + std::pow((*p2).y - (*p1).y, 2));
    double rand_part = -r * len + std::rand() / (RAND_MAX / (2 * r * len));
    h += rand_part;

    point mid_point{ ((*p1).x + (*p2).x) / 2, h };
    auto mid_point_it = points.insert(p2, mid_point);

    midpoint_displacement(p1, mid_point_it, depth - 1);
    midpoint_displacement(mid_point_it, p2, depth - 1);
}

void draw_broken_line() {
    if (points.size() < 3)
        return;
    ImColor c = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    for (auto p2 = std::next(points.begin()); p2 != points.end(); ++p2) {
        auto p1 = std::prev(p2);
        DrawLineBresenham(*p1, *p2, c, c);
    }
}

void draw_broken_line_by_steps() {
    if (points.size() < 3)
        return;
    double pow_of_2 = std::pow(2, cur_depth);
    int step = (points.size() - 1) / pow_of_2;
    ImColor c = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    auto p1 = points.begin();  
    for (int i = 0; i < pow_of_2; ++i) {
        auto p2 = std::next(p1, step);
        DrawLineBresenham(*p1, *p2, c, c);
        p1 = p2;
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


    if (ImGui::Button("Clear Window", ImVec2(100, 50))) {
        is_placing_point = 1;
        step_by_step_drawing = 0;
        cur_depth = 0;
        text_str = "Place 2 points";
        points.clear();
    }

    ImGui::SetCursorPos(ImVec2(120, 27));
    ImGui::SetNextItemWidth(100);
    if (ImGui::Button("Midpoint Disp", ImVec2(100, 50))) {
        if (points.size() < 2)
            text_str = "You have to place 2 points first";
        else {
            is_placing_point = 0;
            step_by_step_drawing = 0;
            cur_depth = 0;
            text_str = "";
            points.erase(std::next(points.begin()), std::prev(points.end()));
            midpoint_displacement(points.begin(), std::prev(points.end()));
        }
    }

    ImGui::SetCursorPos(ImVec2(230, 27));
    ImGui::SetNextItemWidth(100);
    if (ImGui::Button("Draw by steps", ImVec2(100, 50))) {
        if (points.size() < 3)
            text_str = "You have to apply midpoint displacement first";
        else {
            step_by_step_drawing = 1;
            if (cur_depth < max_depth) {
                ++cur_depth;
                if (cur_depth == max_depth)
                    text_str = "The last step is reached";
            }
        }
    }
    
    ImGui::SetCursorPos(ImVec2(340, 27));
    ImGui::SetNextItemWidth(200);
    ImGui::SliderInt("Num of iterations", &max_depth, 1, 18);

    ImGui::SetCursorPos(ImVec2(340, 58));
    ImGui::SetNextItemWidth(200);
    ImGui::SliderFloat("Roughness", &r, 0, 1);

    ImGui::Text("%s", text_str.c_str());
    ImGui::End();
}

int main() {
    setlocale(LC_ALL, "russian");
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(1400, 800, "Lab 5", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwSetWindowSizeLimits(window, 1400, 800, 1400, 800);
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
    std::srand(static_cast<unsigned>(time(0)));

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_UI();

        if (points.size() > 0) 
        {
            DrawPoint((*points.begin()).x, (*points.begin()).y, 1, ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 3);
            DrawPoint((*std::prev(points.end())).x, (*std::prev(points.end())).y, 1, ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 3);

            if (points.size() > 2) {
                if (step_by_step_drawing)
                    draw_broken_line_by_steps();
                else
                    draw_broken_line();
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