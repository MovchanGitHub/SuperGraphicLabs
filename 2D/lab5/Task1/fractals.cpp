#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <list>
#include <stack>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <random>

#define _USE_MATH_DEFINES
#include <math.h>

const int WINDOW_SIZE = 1200;
//std::string fname = "Koch_snowflake.txt";
//std::string fname = "Bush1.txt";
std::string fname = "Gosper_curve.txt";
//std::string fname = "Tree.txt";

class point {
    std::vector<float> cords;

public:
    const float& x;
    const float& y;
    point(float x, float y) : cords{ x, y, 1 }, x(cords[0]), y(cords[1]) {}
    point(const point& p) : cords(p.cords), x(cords[0]), y(cords[1]) {}

    point& operator=(const point& p) {
        cords[0] = p.cords[0];
        cords[1] = p.cords[1];
        return *this;
    }

    point operator*(float num) const {
        point res = *this;
        res.cords[0] *= num;
        res.cords[1] *= num;
        return res;
    }

    point operator+(const point p) const {
        point res = *this;
        res.cords[0] += p.cords[0];
        res.cords[1] += p.cords[1];
        return res;
    }

    void rotate(float angle) {
        angle *= M_PI / 180;
        std::vector<std::vector<float>> m(3, std::vector<float>(3));
        m[0][0] = cos(angle);     m[0][1] = sin(angle);
        m[1][0] = -sin(angle);    m[1][1] = cos(angle);
        m[2][2] = 1;
        this->affine_transformation(m);
    }

    void affine_transformation(std::vector<std::vector<float>>& m) {
        std::vector<float> ncords(3);
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                ncords[j] += cords[k] * m[k][j];
        cords = ncords;
    }
};

ImColor GetGradientColor(ImColor& c1, ImColor& c2, int segments, int segn) {
    ImColor res;
    res.Value.x = c1.Value.x + (c2.Value.x - c1.Value.x) * segn / segments;
    res.Value.y = c1.Value.y + (c2.Value.y - c1.Value.y) * segn / segments;
    res.Value.z = c1.Value.z + (c2.Value.z - c1.Value.z) * segn / segments;
    res.Value.w = c1.Value.w + (c2.Value.w - c1.Value.w) * segn / segments;
    return res;
}

struct LSystem {
    std::string atom;
    std::map<char, std::string> productions;
    point radiusVector;
    float angle;
    float current_angle = 0;
    int times = 0;

    LSystem(std::string atom, float rotation_angle, float angle) :
        atom(atom),
        angle(rotation_angle),
        radiusVector(1, 0)
    { radiusVector.rotate(angle); }

    std::string generate() {
        std::string res = atom;
        int times = this->times;
        while (times--) {
            std::string new_res;
            for (auto c : res)
                if (productions.count(c))
                    new_res.append(productions[c]);
                else new_res.push_back(c);
            swap(res, new_res);
        }
        return res;
    }

    void draw() {
        std::string to_draw = generate();
        float minX = 0;
        float minY = 0;
        float maxX = 0;
        float maxY = 0;
        std::stack<point> st;
        point from(0, 0);
        point direction = radiusVector;
        for (auto c : to_draw) {
            switch (c) {
            case '+':
                direction.rotate(angle);
                break;
            case '-':
                direction.rotate(-angle);
                break;
            case '[':
                st.push(from);
                break;
            case ']':
                from = st.top();
                st.pop();
                break;
            case 'F':
                point to = from + direction;
                minX = std::min(minX, to.x);
                minY = std::min(minY, to.y);
                maxX = std::max(maxX, to.x);
                maxY = std::max(maxY, to.y);
                std::swap(from, to);
                break;
            }
        }
        float scale = (WINDOW_SIZE - 100) / std::max(maxX - minX, maxY - minY);
        direction = radiusVector;
        ImColor color(0, 0, 0, 255);
        from = point(scale * minX - 50, scale * minY - 50) * -1;
        for (auto c : to_draw) {
            switch (c) {
            case '+':
                direction.rotate(angle);
                break;
            case '-':
                direction.rotate(-angle);
                break;
            case '[':
                st.push(from);
                break;
            case ']':
                from = st.top();
                st.pop();
                break;
            case '@':
                
                break;
            case 'F':
                point to = from + direction * scale;
                ImGui::GetForegroundDrawList()->
                    AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), color, 1);
                std::swap(from, to);
                break;
            }
        }
    }
};

void draw_UI() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 100));
    ImGui::Begin("Instruments", NULL,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse);

    ImGui::End();
}

LSystem ReadLSystem() {
    freopen(fname.c_str(), "r", stdin);
    std::string atom; 
    float rotating_angle; 
    float initial_angle;
    char from;
    std::string to;
    std::cin >> atom >> rotating_angle >> initial_angle;
    LSystem res(atom, rotating_angle, initial_angle);
    int n; std::cin >> n;
    while (n--) {
        std::cin >> from >> to;
        res.productions[from] = to;
    }
    int times; std::cin >> times;
    res.times = times;
    return res;
}

int main() {
    srand((unsigned)time(NULL));
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(WINDOW_SIZE, WINDOW_SIZE, "Task 5", NULL, NULL);
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
        //glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static bool drawn = false;
        if (!drawn) {
            static LSystem ls = ReadLSystem();
            ls.draw();
            //drawn = true;
        }
        
        ImGui::Render();
        glClearColor(1.0f, 1.0f, 1.0f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }


    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}