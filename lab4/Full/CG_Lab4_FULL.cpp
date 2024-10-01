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

point poi(-1, -1);

int pos_relative_to_edge(const point& edge_begin, const point& edge_end) {
    point a{ edge_end.x - edge_begin.x,  edge_end.y - edge_begin.y };
    point b{ poi.x - edge_begin.x,  poi.y - edge_begin.y };
    double check = b.y * a.x - b.x * a.y;
    if (std::abs(check) < 0.0001)
        return -1;
    return check > 0;
}

point find_intersect(const point& a, const point& b, const point& c, const point& d)
{
    point n = point(-(d.y - c.y), d.x - c.x);
    point nu = point(-(b.y - a.y), b.x - a.x);

    float t = -(n.x * (a.x - c.x) + n.y * (a.y - c.y)) / (n.x * (b.x - a.x) + n.y * (b.y - a.y));
    float u = -(nu.x * (a.x - c.x) + nu.y * (a.y - c.y)) / (nu.x * (d.x - c.x) + nu.y * (d.y - c.y));

    point res = point(a.x + (int)(t * (b.x - a.x)), a.y + (int)(t * (b.y - a.y)));

    if (t < 0 || t > 1 || u < -1 || u > 0)
        return point{ -1, -1 };

    return res;
}

void draw_intersect_point(int x, int y)
{
    for (int i = -2; i <= 2; i++)
    {
        for (int j = -2; j <= 2; j++)
        {
            DrawPoint(x + i, y + j, 1, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        }
    }
}

void draw_intersect(const point& a, const point& b, const point& c, const point& d)
{
    point n = point(-(d.y - c.y), d.x - c.x);
    point nu = point(-(b.y - a.y), b.x - a.x);

    float t = -(n.x * (a.x - c.x) + n.y * (a.y - c.y)) / (n.x * (b.x - a.x) + n.y * (b.y - a.y));
    float u = -(nu.x * (a.x - c.x) + nu.y * (a.y - c.y)) / (nu.x * (d.x - c.x) + nu.y * (d.y - c.y));

    point res = point(a.x + (int)(t * (b.x - a.x)), a.y + (int)(t * (b.y - a.y)));

    if (t < 0 || t > 1 || u < -1 || u > 0)
    {
        return;
    }

    draw_intersect_point(res.x, res.y);
}


class polygon {
    point center = { 0, 0 };

    double cross_product(const point& edge_begin1, const point& edge_begin2, const point& edge_end2) {
        return (edge_begin2.x - edge_begin1.x) * (edge_end2.y - edge_begin2.y) - (edge_begin2.y - edge_begin1.y) * (edge_end2.x - edge_begin2.x);
    }
public:
    std::list<point> points;
    std::list<std::pair<point, point>> lines;

    double polygon_area() {
        int n = points.size();
        if (n <= 2) return 0;
        double area = 0.0;

        auto it = points.begin();
        while (true) {
            auto nit = it;
            ++nit;
            if (nit == points.end())
                nit = points.begin();
            point& p1 = *it;
            point& p2 = *nit;
            area += p1.x * p2.y - p2.x * p1.y;
            if (nit == points.begin()) break;
            it = nit;
        }

        return area / 2.0;
    }

    point centroid() {
        if (!points.size()) return { 0, 0 };
        if (points.size() == 1) {
            return center = points.front();
        }
        if (points.size() == 2) {
            return center =
            { (points.front().x + points.back().x) / 2,
                (points.front().y + points.back().y) / 2 };
        }

        int n = points.size();
        double area = polygon_area();
        double Cx = 0.0, Cy = 0.0;

        auto it = points.begin();
        while (true) {
            auto nit = it;
            ++nit;
            if (nit == points.end())
                nit = points.begin();
            point& p1 = *it;
            point& p2 = *nit;
            double fact = p1.x * p2.y - p2.x * p1.y;
            Cx += (p1.x + p2.x) * fact;
            Cy += (p1.y + p2.y) * fact;
            if (nit == points.begin()) break;
            it = nit;
        }

        Cx /= (6.0 * area);
        Cy /= (6.0 * area);

        return center = { Cx, Cy };
    }

    void affine_transformation(std::vector<std::vector<double>>& m) {
        for (auto it = points.begin(); it != points.end(); ++it)
            it->affine_transformation(m);
    }

    size_t size() { return points.size(); }

    void add_point(point p) {
        points.push_back(p);
    }

    void clear() { points.clear(); lines.clear(); }

    void draw() {
        if (points.size() == 0) return;
        DrawPoint(center.x, center.y,
            1, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImVec4 border_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        if (points.size() == 1) {
            DrawPoint(points.front().x, points.front().y, 1, border_color);
        }
        else if (points.size() == 2) {
            DrawLineWu(points.front(), points.back(),
                border_color, border_color);
            lines.push_back({ points.front(), points.back() });
        }
        else if (points.size()) {
            auto it = points.begin();
            lines.clear();
            while (true) {
                auto nit = it;
                ++nit;
                if (nit == points.end()) break;
                DrawLineWu(*it, *nit,
                    border_color, border_color);
                lines.push_back({ *it, *nit });
                it = nit;
            }
            DrawLineWu(points.front(), points.back(),
                border_color, border_color);
            lines.push_back({ points.front(), points.back() });
        }
    }

    std::list<point>::iterator find_edge(double x, double y) {
        if (points.size() == 0)
            return points.end();
        for (auto it = points.begin(); it != points.end(); ++it) {
            auto it_next = std::next(it, 1);
            if (it_next == points.end())
                it_next = points.begin();

            ImVec2 vec_line = ImVec2((*it_next).x - (*it).x, (*it_next).y - (*it).y);
            double vec_length = std::sqrt(vec_line.x * vec_line.x + vec_line.y * vec_line.y);
            ImVec2 vec_norm = ImVec2(vec_line.x / vec_length, vec_line.y / vec_length);
            ImVec2 vec_to_mouse = ImVec2(x - (*it).x, y - (*it).y);
            double proj_length = vec_to_mouse.x * vec_norm.x + vec_to_mouse.y * vec_norm.y;

            if (proj_length < 0 || proj_length > vec_length)
                continue;

            ImVec2 closest_to_mouse = ImVec2((*it).x + vec_norm.x * proj_length, (*it).y + vec_norm.y * proj_length);
            ImVec2 dist_to_mouse = ImVec2(x - closest_to_mouse.x, y - closest_to_mouse.y);
            double dist = std::sqrt(dist_to_mouse.x * dist_to_mouse.x + dist_to_mouse.y * dist_to_mouse.y);
            if (dist <= 8)
                return it;
        }
        return points.end();
    }

    bool is_convex() {
        double prev = 0, cur = 0;
        for (auto it = points.begin(); it != points.end(); ++it) {
            auto it_next = std::next(it, 1);
            if (it_next == points.end())
                it_next = points.begin();
            auto it_next_next = std::next(it_next, 1);
            if (it_next_next == points.end())
                it_next_next = points.begin();
            cur = cross_product(*it, *it_next, *it_next_next);
            if (std::abs(cur) > 0.0001f) {
                if (cur * prev < 0)
                    return 0;
                else
                    prev = cur;
            }
        }
        return 1;
    }

    bool does_point_belong_to(const point& p) {
        if (is_convex()) {
            int cur, prev = -1;
            for (auto it = points.begin(); it != points.end(); ++it) {
                auto it_next = std::next(it, 1);
                if (it_next == points.end())
                    it_next = points.begin();
                cur = pos_relative_to_edge(*it, *it_next);
                if (prev != -1 && cur != prev)
                    return 0;
                prev = cur;
            }
            return 1;
        }
        int intersection_cnt = 0;
        for (auto it = points.begin(); it != points.end(); ++it) {
            auto it_next = std::next(it, 1);
            if (it_next == points.end())
                it_next = points.begin();
            if (p.y <= std::max((*it).y, (*it_next).y) && p.y > std::min((*it).y, (*it_next).y)
                && p.x <= std::max((*it).x, (*it_next).x)) {
                double intersection = find_intersect(p, point{ 3000, p.y }, (*it), (*it_next)).x;
                if (p.x <= intersection || (*it).x == (*it_next).x)
                    ++intersection_cnt;
            }
        }
        return intersection_cnt % 2;
    }
};

class line {
    point center = { 0, 0 };
    point mouse_point = point(0, 0);

public:
    std::list<point> points;
    void add_point(point p) {
        points.push_back(p);
    }

    void clear() { points.clear(); }

    void draw(GLFWwindow* window, const polygon& pol) {
        if (points.size() == 0) return;
        DrawPoint(center.x, center.y,
            1, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImVec4 border_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        if (points.size() == 1) {
            DrawPoint(points.front().x, points.front().y, 1, border_color);

            glfwGetCursorPos(window, &mouse_point.x, &mouse_point.y);

            DrawLineWu(points.front(), mouse_point,
                border_color, border_color);
            for (auto l : pol.lines)
            {
                draw_intersect(points.front(), mouse_point, l.first, l.second);
            }
        }
        else if (points.size() == 2) {
            DrawLineWu(points.front(), points.back(),
                border_color, border_color);
            for (auto l : pol.lines)
            {
                draw_intersect(points.front(), points.back(), l.first, l.second);
            }
        }
        else {
            clear();
        }
    }
};

polygon pol;
bool is_placing_point = 0, is_choosing_edge = 0, is_drawing_line = 0;
std::string text_str = "";
line ln;

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

        if (is_choosing_edge) {
            auto it = pol.find_edge(xpos, ypos);
            if (it != pol.points.end()) {
                int pos = pos_relative_to_edge(*it, (std::next(it, 1) == pol.points.end()) ? *pol.points.begin() : *std::next(it, 1));
                if (pos == 0)
                    text_str = "The point is to the left of the selected edge";
                else if (pos == 1)
                    text_str = "The point is to the right of the selected edge";
                else
                    text_str = "The point is on the same line with the selected edge";
            }
            return;
        }

        if (is_placing_point) {
            poi = point{ xpos, ypos };
            if (pol.size() < 3)
                return;
            if (pol.does_point_belong_to(poi))
                text_str = "The point belongs to the polygon";
            else
                text_str = "The point doesn't belong to the polygon";
            return;
        }

        if (is_drawing_line)
        {
            ln.add_point({ xpos, ypos });
            return;
        }

        pol.add_point({ xpos, ypos });
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        pol.clear();
        poi = point{ -1, -1 };
        text_str = "";
        is_placing_point = 0;
        is_choosing_edge = 0;
    }
}

std::vector<std::vector<double>> offset_matr(3, std::vector<double>(3));
std::vector<std::vector<double>> rotate_matr(3, std::vector<double>(3));
std::vector<std::vector<double>> scalin_matr(3, std::vector<double>(3));

void initialize_matrixes() {
    offset_matr[0][0] = 1;   offset_matr[0][1] = 0;   offset_matr[0][2] = 0;
    offset_matr[1][0] = 0;   offset_matr[1][1] = 1;   offset_matr[1][2] = 0;
    offset_matr[2][0] = 0;   offset_matr[2][1] = 0;   offset_matr[2][2] = 1;

    rotate_matr[0][0] = 1;   rotate_matr[0][1] = 0;   rotate_matr[0][2] = 0;
    rotate_matr[1][0] = 0;   rotate_matr[1][1] = 1;   rotate_matr[1][2] = 0;
    rotate_matr[2][0] = 0;   rotate_matr[2][1] = 0;   rotate_matr[2][2] = 1;

    scalin_matr[0][0] = 1;   scalin_matr[0][1] = 0;   scalin_matr[0][2] = 0;
    scalin_matr[1][0] = 0;   scalin_matr[1][1] = 1;   scalin_matr[1][2] = 0;
    scalin_matr[2][0] = 0;   scalin_matr[2][1] = 0;   scalin_matr[2][2] = 1;
}

std::vector<std::vector<double>> matr_mult(
    std::vector<std::vector<double>>& m1,
    std::vector<std::vector<double>>& m2) {
    std::vector<std::vector<double>> res(3, std::vector<double>(3));
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                res[i][j] += m1[i][k] * m2[k][j];
    return res;
}

void general_transformation(point p, std::vector<std::vector<double>> matr) {
    double dx = offset_matr[2][0];
    double dy = offset_matr[2][0];

    offset_matr[2][0] = -p.x;
    offset_matr[2][1] = -p.y;

    matr = matr_mult(offset_matr, matr);
    offset_matr[2][0] = p.x;
    offset_matr[2][1] = p.y;
    matr = matr_mult(matr, offset_matr);
    pol.affine_transformation(matr);

    offset_matr[2][0] = dx;
    offset_matr[2][1] = dy;
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
        pol.clear();
        ln.clear();
        poi = point{ -1, -1 };
        text_str = "";
        is_placing_point = 0;
        is_choosing_edge = 0;
    }

    ImGui::SetCursorPos(ImVec2(120, 27));
    static int dx = 0;
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("dx", &dx))
        offset_matr[2][0] = dx;
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(120, 58));
    static int dy = 0;
    if (ImGui::InputInt("dy", &dy))
        offset_matr[2][1] = -dy;
    ImGui::SetCursorPos(ImVec2(250, 27));
    if (ImGui::Button("Shift", ImVec2(100, 50))) {
        poi = point{ -1, -1 };
        text_str = "";
        is_placing_point = 0;
        is_choosing_edge = 0;
        pol.affine_transformation(offset_matr);
    }

    static int px = 0;
    static int py = 0;
    static bool is_center = true;
    static bool is_not_center = false;

    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(360, 27));
    static float alpha = 0;
    if (ImGui::InputFloat("a", &alpha)) {
        double rad = -alpha * (M_PI / 180.0);
        rotate_matr[0][0] = cos(rad);     rotate_matr[0][1] = sin(rad);
        rotate_matr[1][0] = -sin(rad);    rotate_matr[1][1] = cos(rad);
    }
    ImGui::SetCursorPos(ImVec2(480, 27));
    if (ImGui::Button("Rotate", ImVec2(100, 50))) {
        poi = point{ -1, -1 };
        text_str = "";
        is_placing_point = 0;
        is_choosing_edge = 0;
        point p = is_center ? pol.centroid() : point(px, py);
        general_transformation(p, rotate_matr);
    }

    ImGui::SetCursorPos(ImVec2(590, 27));
    ImGui::SetNextItemWidth(100);
    static float kx = 1;
    if (ImGui::InputFloat("kx", &kx))
        scalin_matr[0][0] = kx;
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(590, 58));
    static float ky = 1;
    if (ImGui::InputFloat("ky", &ky))
        scalin_matr[1][1] = ky;
    ImGui::SetCursorPos(ImVec2(720, 27));
    if (ImGui::Button("Scale", ImVec2(100, 50))) {
        poi = point{ -1, -1 };
        text_str = "";
        is_placing_point = 0;
        is_choosing_edge = 0;
        point p = is_center ? pol.centroid() : point(px, py);
        general_transformation(p, scalin_matr);
    }


    ImGui::SetCursorPos(ImVec2(830, 27));
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("px", &px);
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(830, 58));
    ImGui::InputInt("py", &py);


    ImGui::SetCursorPos(ImVec2(960, 27));
    if (ImGui::Checkbox("center", &is_center))
        is_not_center = !is_center;
    ImGui::SetCursorPos(ImVec2(960, 58));
    if (ImGui::Checkbox("point", &is_not_center))
        is_center = !is_not_center;

    ImGui::SetCursorPos(ImVec2(1030, 27));
    if (ImGui::Button("Draw Line", ImVec2(100, 50))) {
        is_placing_point = 0;
        is_choosing_edge = 0;
        poi = point{ -1, -1 };
        if (is_drawing_line)
        {
            text_str = "";
            ln.clear();
        }
        else 
        {
            text_str = "Draw Line";
        }
        is_drawing_line = !is_drawing_line;
    }

    ImGui::SetCursorPos(ImVec2(1140, 27));
    ImGui::SetNextItemWidth(100);
    if (ImGui::Button("Place Point", ImVec2(100, 50))) {
        ln.clear();
        is_drawing_line = 0;
        is_placing_point = !is_placing_point;
        is_choosing_edge = 0;
        poi = point{ -1, -1 };
        if (is_placing_point)
            text_str = "Place a point";
        else
            text_str = "";
    }
    ImGui::SetCursorPos(ImVec2(1250, 27));
    ImGui::SetNextItemWidth(100);
    if (ImGui::Button("Left or right", ImVec2(100, 50))) {
        ln.clear();
        is_drawing_line = 0;
        if (pol.size() < 2 || poi.x == -1)
            text_str = "No edges available for selection or you haven't placed a point";
        else {
            is_placing_point = is_choosing_edge;
            is_choosing_edge = !is_choosing_edge;
            if (is_choosing_edge)
                text_str = "Choose an edge";
            else
                text_str = "Place a point";
        }
    }
    ImGui::Text("%s", text_str.c_str());
    ImGui::End();


    ImGui::GetForegroundDrawList()->
        AddRectFilled(ImVec2(px - 5, py - 5), ImVec2(px + 5, py + 5),
            ImColor(0.0f, 1.0f, 1.0f, 1.0f));
}


int main() {
    setlocale(LC_ALL, "russian");
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(1400, 800, "Lab4", NULL, NULL);
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

    initialize_matrixes();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_UI();

        pol.draw();
        if (poi.x != -1)
            DrawPoint(poi.x, poi.y, 1, ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 4);

        if (is_drawing_line)
        {
            ln.draw(window, pol);
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