#include <iostream>
#include <vector>
#include <list>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define _USE_MATH_DEFINES
#include <math.h>

double h = 100;

using uint = unsigned int;

class point {
    std::vector<double> cords;

public:
    double& x, y, z;
    point(double x = 0, double y = 0, double z = 0) : cords{ x, y, z, 1 }, x(cords[0]), y(cords[1]), z(cords[2]) {}
    point(const point& p) : cords(p.cords), x(cords[0]), y(cords[1]), z(cords[2]) {}
    point& operator=(const point& p) {
        cords = p.cords;
        return *this;
    }
    void affine_transformation(std::vector<std::vector<double>>& m) {
        std::vector<double> ncords(4);
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k)
                ncords[j] += cords[k] * m[k][j];
        cords = ncords;
    }
};

void DrawPoint(int x, int y, float intencity, ImColor c) {
    c.Value.w *= intencity;
    ImGui::GetForegroundDrawList()->
        AddRectFilled(ImVec2(x, y), ImVec2(x + 1, y + 1), c);
}
void DrawPoint(const point& p, float intencity, ImColor c) {
    DrawPoint(p.x, p.y, intencity, c);
}
void DrawLineWu(point p0, point p1, ImColor c) {
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
        }
        DrawPoint(x0, y0, 1, c);
        float y = y0 + gradient;
        for (int x = x0 + 1; x <= x1 - 1; ++x) {
            DrawPoint(x, (int)y, 1 - (y - (int)y), c);
            DrawPoint(x, (int)y + 1, y - (int)y, c);
            y += gradient;
        }
        DrawPoint(x1, y1, 1, c);
    }
    else {
        gradient = (float)dx / dy;
        if (y0 > y1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        DrawPoint(x0, y0, 1, c);
        float x = x0 + gradient;
        for (int y = y0 + 1; y <= y1 - 1; ++y) {
            DrawPoint((int)x, y, 1 - (x - (int)x), c);
            DrawPoint((int)x + 1, y, x - (int)x, c);
            x += gradient;
        }
        DrawPoint(x1, y1, 1, c);
    }
}

class polyhedron {
    std::vector<point> vertices;

    struct polygon {
        std::list<point*> vertices;

        void draw() const {
            if (vertices.size() == 0) return;
            ImVec4 border_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            if (vertices.size() == 1)
                DrawPoint(*vertices.front(), 1, border_color);
            else if (vertices.size() == 2) {
                DrawLineWu(*vertices.front(), *vertices.back(),
                    border_color);
            }
            else if (vertices.size()) {
                auto it = vertices.begin();
                while (true) {
                    auto nit = it;
                    ++nit;
                    if (nit == vertices.end()) break;
                    DrawLineWu(**it, **nit,
                        border_color);
                    it = nit;
                }
                DrawLineWu(*vertices.front(), *vertices.back(),
                    border_color);
            }
        }

        void add_point(point* p) {
            vertices.push_back(p);
        }
    };

    std::vector<polygon> faces;

public:
    polyhedron(uint number_of_faces): faces(number_of_faces) {}

    void affine_transformation(std::vector<std::vector<double>>& m) {
        for (auto it = vertices.begin(); it != vertices.end(); ++it)
            it->affine_transformation(m);
    }
    
    void draw() const {
        for (const polygon& face : faces)
            face.draw();
    }

    uint add_point(const point& p) {
        vertices.push_back(p);
        return vertices.size() - 1;
    }

    void tie_vertex_to_face(uint vertex_index, uint face_index) {
        pol.faces[face_index].add_point(&vertices[vertex_index]);
    }
} pol(20);

std::vector<std::vector<double>> offset_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> rotate_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> scalin_matr(4, std::vector<double>(4));

void initialize_matrixes() {
    offset_matr[0][0] = offset_matr[1][1] = offset_matr[2][2] = offset_matr[3][3] = 1;
    scalin_matr[0][0] = scalin_matr[1][1] = scalin_matr[2][2] = scalin_matr[3][3] = 1;
}

std::vector<std::vector<double>> matr_mult(
    std::vector<std::vector<double>>& m1,
    std::vector<std::vector<double>>& m2) {
    std::vector<std::vector<double>> res(4, std::vector<double>(4));
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k)
                res[i][j] += m1[i][k] * m2[k][j];
    return res;
}

std::vector<std::vector<double>> general_transformation(point p, std::vector<std::vector<double>> matr) {
    double dx = offset_matr[3][0];
    double dy = offset_matr[3][1];
    double dz = offset_matr[3][2];

    offset_matr[3][0] = -p.x;
    offset_matr[3][1] = -p.y;
    offset_matr[3][2] = -p.z;
    matr = matr_mult(offset_matr, matr);

    offset_matr[3][0] = +p.x;
    offset_matr[3][1] = +p.y;
    offset_matr[3][2] = +p.z;
    matr = matr_mult(matr, offset_matr);

    offset_matr[3][0] = dx;
    offset_matr[3][1] = dy;
    offset_matr[3][2] = dz;

    return matr;
}

void build_polyhedron() {
    double r = h / (2 * sin(M_PI / 5) * sin(M_PI / 3));
    point top{r, h / 2, 0};
    point bottom{r, -h / 2, 0};
    std::vector<std::vector<double>> rotate_y36(4, std::vector<double>(4));
    rotate_y36[0][0] = rotate_y36[2][2] = cos(M_PI / 5);
    rotate_y36[2][0] = sin(M_PI / 5);
    rotate_y36[0][2] = -sin(M_PI / 5);
    rotate_y36[1][1] = rotate_y36[3][3] = 1;
    bottom.affine_transformation(rotate_y36);

    std::vector<std::vector<double>> rotate_y72(4, std::vector<double>(4));
    rotate_y72[0][0] = rotate_y72[2][2] = cos(2 * M_PI / 5);
    rotate_y72[2][0] = sin(2 * M_PI / 5);
    rotate_y72[0][2] = -sin(2 * M_PI / 5);
    rotate_y72[1][1] = rotate_y72[3][3] = 1;
    for (int i = 0; i < 5; ++i) {
        top.affine_transformation(rotate_y72);
        bottom.affine_transformation(rotate_y72);
        pol.add_point(top);
        pol.add_point(bottom);
    }

    for (int i = 0; i < 10; ++i) {
        pol.tie_vertex_to_face(i, i);
        pol.tie_vertex_to_face((i + 1) % 10, i);
        pol.tie_vertex_to_face((i + 2) % 10, i);
    }
    top = { 0, h / 2 + r * sqrt(4 * sin(M_PI / 5) * sin(M_PI / 5) - 1), 0 };
    bottom = { 0, -(h / 2 + r * sqrt(4 * sin(M_PI / 5) * sin(M_PI / 5) - 1)), 0 };
    pol.add_point(top);
    pol.add_point(bottom);
    for (int i = 0; i < 5; ++i) {
        pol.tie_vertex_to_face(2 * i, 10 + 2 * i);
        pol.tie_vertex_to_face((2 * i) % 10, 10 + 2 * i);
        pol.tie_vertex_to_face(10, 10 + 2 * i);

        pol.tie_vertex_to_face(2 * i + 1, 11 + 2 * i);
        pol.tie_vertex_to_face((2 * i + 1) % 10, 11 + 2 * i);
        pol.tie_vertex_to_face(11, 11 + 2 * i);
    }
}

void build_cube() {
    pol.add_point({300, 300, 0});
    pol.add_point({300, 700, 0});
    pol.add_point({700, 700, 0});
    pol.add_point({700, 300, 0});

    pol.add_point({ 200, 400, 400 });
    pol.add_point({ 200, 800, 400 });
    pol.add_point({ 600, 800, 400 });
    pol.add_point({ 600, 400, 400 });

    pol.tie_vertex_to_face(0, 0);
    pol.tie_vertex_to_face(1, 0);
    pol.tie_vertex_to_face(2, 0);
    pol.tie_vertex_to_face(3, 0);

    pol.tie_vertex_to_face(4, 1);
    pol.tie_vertex_to_face(5, 1);
    pol.tie_vertex_to_face(6, 1);
    pol.tie_vertex_to_face(7, 1);

    pol.tie_vertex_to_face(4, 2);
    pol.tie_vertex_to_face(0, 2);
    pol.tie_vertex_to_face(1, 2);
    pol.tie_vertex_to_face(5, 2);

    pol.tie_vertex_to_face(7, 3);
    pol.tie_vertex_to_face(3, 3);
    pol.tie_vertex_to_face(2, 3);
    pol.tie_vertex_to_face(6, 3);

    pol.tie_vertex_to_face(7, 4);
    pol.tie_vertex_to_face(3, 4);
    pol.tie_vertex_to_face(0, 4);
    pol.tie_vertex_to_face(4, 4);

    pol.tie_vertex_to_face(5, 5);
    pol.tie_vertex_to_face(1, 5);
    pol.tie_vertex_to_face(2, 5);
    pol.tie_vertex_to_face(6, 5);
}

void change_rotate_mart(double teta, int rotate_index) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            rotate_matr[i][j] = 0;
    for (int i = 0; i < 4; ++i)
        rotate_matr[i][i] = 1;
    switch (rotate_index) {
    case 0:
        rotate_matr[1][1] = cos(teta);
        rotate_matr[1][2] = sin(teta);
        rotate_matr[2][1] = -sin(teta);
        rotate_matr[2][2] = cos(teta);
        break;
    case 1:
        rotate_matr[0][0] = cos(teta);
        rotate_matr[0][2] = -sin(teta);
        rotate_matr[2][0] = sin(teta);
        rotate_matr[2][2] = cos(teta);
        break;
    case 2:
        rotate_matr[0][0] = cos(teta);
        rotate_matr[0][1] = sin(teta);
        rotate_matr[1][0] = -sin(teta);
        rotate_matr[1][1] = cos(teta);
        break;
    }
}

void draw_UI() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 100));
    ImGui::Begin("Instruments", NULL,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse);


    if (ImGui::Button("Clear Window", ImVec2(100, 50))) {

    }


    ImGui::SetCursorPos(ImVec2(120, 27));
    static int dx = 0;
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("dx", &dx))
        offset_matr[3][0] = dx;
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(120, 50));
    static int dy = 0;
    if (ImGui::InputInt("dy", &dy))
        offset_matr[3][1] = -dy;
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(120, 73));
    static int dz = 0;
    if (ImGui::InputInt("dz", &dz))
        offset_matr[3][2] = -dz;
    ImGui::SetCursorPos(ImVec2(250, 27));
    if (ImGui::Button("Shift", ImVec2(100, 50))) {
        pol.affine_transformation(offset_matr);
    }

    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(360, 27));
    static float alpha = 0;
    static double teta = 0;
    static int rotate_index = -1;
    if (ImGui::InputFloat("a", &alpha)) {
        teta = -alpha * (M_PI / 180.0);
        change_rotate_mart(teta, rotate_index);
    }
    ImGui::SetCursorPos(ImVec2(360, 50));
    if (ImGui::RadioButton("X", &rotate_index, 0)) {
        change_rotate_mart(teta, rotate_index);
    }
    ImGui::SetCursorPos(ImVec2(395, 50));
    if (ImGui::RadioButton("Y", &rotate_index, 1)) {
        change_rotate_mart(teta, rotate_index);
    }
    ImGui::SetCursorPos(ImVec2(430, 50));
    if (ImGui::RadioButton("Z", &rotate_index, 2)) {
        change_rotate_mart(teta, rotate_index);
    }
    ImGui::SetCursorPos(ImVec2(480, 27));
    if (ImGui::Button("Rotate", ImVec2(100, 50))) {
        auto m = general_transformation({ 300, 300, 300 }, rotate_matr);
        pol.affine_transformation(m);
    }

    ImGui::SetCursorPos(ImVec2(590, 27));
    ImGui::SetNextItemWidth(100);
    static float kx = 1;
    if (ImGui::InputFloat("kx", &kx))
        scalin_matr[0][0] = kx;
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(590, 50));
    static float ky = 1;
    if (ImGui::InputFloat("ky", &ky))
        scalin_matr[1][1] = ky;
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(590, 73));
    static float kz = 1;
    if (ImGui::InputFloat("kz", &kz))
        scalin_matr[2][2] = kz;
    ImGui::SetCursorPos(ImVec2(720, 27));
    if (ImGui::Button("Scale", ImVec2(100, 50))) {
        pol.affine_transformation(scalin_matr);
    }

    ImGui::End();
}

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(1500, 1000, "Task 2", NULL, NULL);
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

    initialize_matrixes();
    build_polyhedron();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_UI();
        pol.draw();

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