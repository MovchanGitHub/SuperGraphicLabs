#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <string>

double h = 100;

using uint = unsigned int;

class point {
    std::vector<double> cords;

public:
    double& x, & y, & z;
    point(double x = 0, double y = 0, double z = 0) : cords{ x, y, z, 1 }, x(cords[0]), y(cords[1]), z(cords[2]) {}
    point(const point& p) : cords(p.cords), x(cords[0]), y(cords[1]), z(cords[2]) {}
    point& operator=(const point& p) {
        cords = p.cords;
        return *this;
    }

    void normalize() {
        if (cords[3] != 0) {
            cords[0] /= cords[3];
            cords[1] /= cords[3];
            cords[2] /= cords[3];
            cords[3] = 1;
        }
        x = cords[0];
        y = cords[1];
        z = cords[2];
    }

    void affine_transformation(std::vector<std::vector<double>>& m) {
        std::vector<double> ncords(4);
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k)
                ncords[j] += cords[k] * m[k][j];
        cords = ncords;
        normalize();
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

std::vector<std::vector<double>> offset_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> rotate_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> scalin_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> view_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> projection_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> base_proj_matr(4, std::vector<double>(4));

void initialize_matrixes() {
    offset_matr[0][0] = offset_matr[1][1] = offset_matr[2][2] = offset_matr[3][3] = 1;
    scalin_matr[0][0] = scalin_matr[1][1] = scalin_matr[2][2] = scalin_matr[3][3] = 1;
    view_matr[0][0] = view_matr[1][1] = view_matr[2][2] = view_matr[3][3] = 1;
    projection_matr[0][0] = projection_matr[1][1] = projection_matr[2][2] = projection_matr[3][3] = 1;
    base_proj_matr = projection_matr;
    view_matr[3][0] = 950;
    view_matr[3][1] = 550;
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

class polyhedron {
    std::vector<point> vertices;
    std::vector<point> view_vertices;

    struct polygon {
        std::list<point*> vertices;
        std::vector<uint> vert_indices;

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

        void add_point(point* p, uint ind) {
            vertices.push_back(p);
            vert_indices.push_back(ind);
        }
    };

public:
    std::vector<polygon> faces;

    polyhedron(uint number_of_faces = 0) : faces(number_of_faces) {}

    void apply_view_matr(std::vector<std::vector<double>>& view_m) {
        view_vertices.assign(vertices.begin(), vertices.end());
        for (auto it = view_vertices.begin(); it != view_vertices.end(); ++it)
            it->affine_transformation(view_m);
    }

    void affine_transformation(std::vector<std::vector<double>>& m, std::vector<std::vector<double>>& projection, std::vector<std::vector<double>>& view) {
        for (auto it = vertices.begin(); it != vertices.end(); ++it)
            it->affine_transformation(m);
        auto proj_view = matr_mult(projection, view);
        this->apply_view_matr(proj_view);
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
        pol.faces[face_index].add_point(&view_vertices[vertex_index], vertex_index);
    }

    void add_vertex_to_top_face(uint vertex_index) {
        pol.faces.back().add_point(&view_vertices[vertex_index], vertex_index);
    }

    void add_face() {
        pol.faces.emplace_back();
    }

    point centroid() const {
        double x_sum = 0, y_sum = 0, z_sum = 0;
        for (const auto& vertex : vertices) {
            x_sum += vertex.x;
            y_sum += vertex.y;
            z_sum += vertex.z;
        }
        size_t count = vertices.size();
        return { x_sum / count, y_sum / count, z_sum / count };
    }

    void clear() {
        int face_count = faces.size();
        faces.clear();
        faces = std::vector<polygon>(face_count);
        vertices.clear();
        view_vertices.clear();
    }

    void save_to_obj(const std::string& file_name) {
        std::ofstream file(file_name);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << file_name << std::endl;
            return;
        }
        for (const auto& v : vertices)
            file << "v " << v.x << ' ' << v.y << ' ' << v.z << '\n';
        for (const polygon& face : faces) {
            file << "f ";
            for (uint index : face.vert_indices)
                file << index + 1 << ' ';
            file << '\n';
        }
        file.close();
    }

    void load_from_obj(const std::string& file_name) {
        std::ifstream file(file_name);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << file_name << std::endl;
            return;
        }

        clear();
        faces.clear();

        uint face_index = 0;
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string type;
            iss >> type;
            if (type.empty() || type[0] == '#') {
                continue;
            }
            if (type == "v") {
                float x, y, z;
                iss >> x >> y >> z;
                add_point({ x, y, z });
            }
            else if (type == "f")
                break;
        }

        apply_view_matr(view_matr);
        
        do {
            std::istringstream iss(line);
            iss.ignore();
            int vertex_ind, norm_ind, tex_coord_ind;
            faces.push_back(polygon());
            while (iss >> vertex_ind) {
                vertex_ind--;
                tie_vertex_to_face(vertex_ind, face_index);

                char ch1 = iss.peek();
                if (ch1 == '/') {
                    iss.ignore();
                    ch1 = iss.peek();
                    if (ch1 == '/') {
                        iss.ignore();
                        iss >> norm_ind;
                    }
                    else if (isdigit(ch1)) {
                        iss >> tex_coord_ind;
                        ch1 = iss.peek();
                        if (ch1 == '/') {
                            iss.ignore();
                            iss >> norm_ind;
                        }
                    }
                }
                
                ch1 = iss.peek();
                if (ch1 == ' ')
                    iss.ignore();
            }
            face_index++;
        } while (std::getline(file, line));

        file.close();
    }
} pol(0);

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

void build_icosahedron() {
    pol = polyhedron(20);

    double r = h / (2 * sin(M_PI / 5) * sin(M_PI / 3));
    point top{ r, h / 2, 0 };
    point bottom{ r, -h / 2, 0 };
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

    top = { 0, h / 2 + r * sqrt(4 * sin(M_PI / 5) * sin(M_PI / 5) - 1), 0 };
    bottom = { 0, -(h / 2 + r * sqrt(4 * sin(M_PI / 5) * sin(M_PI / 5) - 1)), 0 };
    pol.add_point(top);
    pol.add_point(bottom);
    pol.apply_view_matr(view_matr);
    for (int i = 0; i < 10; ++i) {
        pol.tie_vertex_to_face(i, i);
        pol.tie_vertex_to_face((i + 1) % 10, i);
        pol.tie_vertex_to_face((i + 2) % 10, i);
    }
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
    pol = polyhedron(6);

    pol.add_point({ -50, -50, -50 });
    pol.add_point({ +50, -50, -50 });
    pol.add_point({ +50, +50, -50 });
    pol.add_point({ -50, +50, -50 });

    pol.add_point({ -50, -50, +50 });
    pol.add_point({ +50, -50, +50 });
    pol.add_point({ +50, +50, +50 });
    pol.add_point({ -50, +50, +50 });

    pol.apply_view_matr(view_matr);

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

void build_tetrahedron() {
    pol = polyhedron(4);

    pol.add_point({ -50, -50, -50 });
    pol.add_point({ +50, +50, -50 });

    pol.add_point({ +50, -50, +50 });
    pol.add_point({ -50, +50, +50 });

    pol.apply_view_matr(view_matr);

    pol.tie_vertex_to_face(0, 0);
    pol.tie_vertex_to_face(1, 0);
    pol.tie_vertex_to_face(2, 0);

    pol.tie_vertex_to_face(1, 1);
    pol.tie_vertex_to_face(2, 1);
    pol.tie_vertex_to_face(3, 1);

    pol.tie_vertex_to_face(2, 2);
    pol.tie_vertex_to_face(3, 2);
    pol.tie_vertex_to_face(0, 2);

    pol.tie_vertex_to_face(3, 3);
    pol.tie_vertex_to_face(0, 3);
    pol.tie_vertex_to_face(1, 3);
}

void build_dodecahedron() {
    build_icosahedron();
    std::vector<point> points;
    for (auto& face : pol.faces) {
        point np = { 0, 0, 0 };
        int cnt = 0;
        for (auto p : face.vertices) {
            ++cnt;
            np.x += p->x;
            np.y += p->y;
            np.z += p->z;
        }
        np.x /= cnt;
        np.y /= cnt;
        np.z /= cnt;
        points.push_back(np);
    }
    pol = polyhedron(12);
    for (auto p : points)
        pol.add_point(p);
    pol.apply_view_matr(view_matr);
    pol.tie_vertex_to_face(8, 0);
    pol.tie_vertex_to_face(0, 0);
    pol.tie_vertex_to_face(2, 0);
    pol.tie_vertex_to_face(12, 0);
    pol.tie_vertex_to_face(10, 0);
    //for (int i = 0; i < 10; ++i) {
    //    pol.tie_vertex_to_face((i + 9) % 10, i);
    //    pol.tie_vertex_to_face(i, i);
    //    pol.tie_vertex_to_face((i + 1) % 10, i);
    //    pol.tie_vertex_to_face(10 + i, i);
    //    pol.tie_vertex_to_face(10 + (i + 1) % 10, i);
    //}
    for (int i = 10; i < 20; i += 2)
        pol.tie_vertex_to_face(i, 10);
    for (int i = 10; i < 20; i += 2)
        pol.tie_vertex_to_face(i + 1, 11);
}

void build_octahedron() {
    pol = polyhedron(8);

    pol.add_point({ 0, -50, 0 });
    pol.add_point({ +50, 0, 0 });
    pol.add_point({ 0, +50, 0 });
    pol.add_point({ -50, 0, 0 });
    pol.add_point({ 0, 0, +50 });
    pol.add_point({ 0, 0, -50 });

    pol.apply_view_matr(view_matr);

    pol.tie_vertex_to_face(0, 0);
    pol.tie_vertex_to_face(1, 0);
    pol.tie_vertex_to_face(4, 0);

    pol.tie_vertex_to_face(4, 1);
    pol.tie_vertex_to_face(1, 1);
    pol.tie_vertex_to_face(2, 1);

    pol.tie_vertex_to_face(3, 2);
    pol.tie_vertex_to_face(4, 2);
    pol.tie_vertex_to_face(2, 2);

    pol.tie_vertex_to_face(0, 3);
    pol.tie_vertex_to_face(4, 3);
    pol.tie_vertex_to_face(3, 3);

    pol.tie_vertex_to_face(5, 4);
    pol.tie_vertex_to_face(1, 4);
    pol.tie_vertex_to_face(0, 4);

    pol.tie_vertex_to_face(5, 5);
    pol.tie_vertex_to_face(3, 5);
    pol.tie_vertex_to_face(0, 5);

    pol.tie_vertex_to_face(5, 5);
    pol.tie_vertex_to_face(2, 5);
    pol.tie_vertex_to_face(1, 5);

    pol.tie_vertex_to_face(5, 5);
    pol.tie_vertex_to_face(3, 5);
    pol.tie_vertex_to_face(2, 5);
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

bool isDrawingLine = false;
std::vector<std::vector<double>> rotate_matr_line(4, std::vector<double>(4));

void change_rotate_around_line(double alpha, point p1, point p2) {

    std::vector<double> direct_vect = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
    double len = sqrt(pow(direct_vect[0], 2) + pow(direct_vect[1], 2) + pow(direct_vect[2], 2));
    std::vector<double> unit_vector = { direct_vect[0] / len, direct_vect[1] / len, direct_vect[2] / len };
    point pointA = p1;
    double a = pointA.x;
    double b = pointA.y;
    double c = pointA.z;
    double l = unit_vector[0];
    double m = unit_vector[1];
    double n = unit_vector[2];
    double d = sqrt(pow(m, 2) + pow(n, 2));

    rotate_matr_line = {
    {pow(l, 2) + (1 - pow(l, 2)) * cos(alpha), l * (1 - cos(alpha)) * m + n * sin(alpha), l * (1 - cos(alpha)) * n - m * sin(alpha), 0},
    {l * (1 - cos(alpha)) * m - n * sin(alpha), pow(m, 2) + (1 - pow(m, 2)) * cos(alpha), m * (1 - cos(alpha)) * n + l * sin(alpha), 0},
    {l * (1 - cos(alpha)) * n + m * sin(alpha), m * (1 - cos(alpha)) * n - l * sin(alpha), pow(n, 2) + (1 - pow(n, 2)) * cos(alpha), 0},
    {0, 0, 0, 1}
    };
}

bool isAxo = false;
bool isPerspec = false;

std::vector<std::vector<double>> create_axo_matrix(double psi, double phi) {
    double sin_psi = std::sin(psi);
    double cos_psi = std::cos(psi);
    double sin_phi = std::sin(phi);
    double cos_phi = std::cos(phi);

    std::vector<std::vector<double>> axo_matrix = {
        {cos_psi, sin_phi * sin_psi, 0, 0},
        {0, cos_phi, 0, 0},
        {sin_psi, -sin_phi * cos_psi, 0, 0},
        {0, 0, 0, 1}
    };

    return axo_matrix;
}

std::vector<std::vector<double>> create_perspective_matrix(double c) {
    std::vector<std::vector<double>> perspec_matrix = {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, -1 / c },
        { 0, 0, 0, 1}
    };

    return perspec_matrix;
}


int number_of_revolutions = 0;
std::vector<point> plane_figure{ {0, 0, 0}, {50, 0, 0}, {0, -100, 0} };
std::pair<point, point> axe_of_rotation = { {0, 0, 0}, {0, 1, 0} };
void build_solid_of_revolution() {
    int n = number_of_revolutions;
    if (n < 3) return;
    double rotate_angle = 2 * M_PI / n;
    int m = plane_figure.size();

    change_rotate_around_line(rotate_angle,
        axe_of_rotation.first, axe_of_rotation.second);
    pol = polyhedron();
    for (int i = 0; i < n; ++i)
        for (auto& vertex : plane_figure) {
            pol.add_point(vertex);
            vertex.affine_transformation(rotate_matr_line);
        }
    pol.apply_view_matr(view_matr);
    int ind = 0;
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m; ++i) {
            pol.add_face();
            int p1 = i + j * m;
            int p2 = (i + 1) % m + j * m;
            int p3 = i + ((j + 1) % n) * m;
            int p4 = (i + 1) % m + ((j + 1) % n) * m;
            pol.add_vertex_to_top_face(p1);
            pol.add_vertex_to_top_face(p2);
            pol.add_vertex_to_top_face(p4);
            pol.add_vertex_to_top_face(p3);
            ++ind;
        }
    }
}

void change_polyhedron(int choose) {
    switch (choose) {
    case 6:
        build_cube();
        break;
    case 4:
        build_tetrahedron();
        break;
    case 8:
        build_octahedron();
        break;
    case 20:
        build_icosahedron();
        break;
    case 12:
        build_dodecahedron();
        break;
    case -1:
        build_solid_of_revolution();
        break;
    }
}

typedef double (*Function) (double x, double y);
double sin_m_cos_func(double x, double y) { return sin(x) * cos(y); }
double sin_p_cos_func(double x, double y) { return sin(x) + cos(y); }
double x_m_y_func(double x, double y) { return x * y; }
double x2_p_y2_func(double x, double y) { return x * x + y * y; }
double x2_m_y2_func(double x, double y) { return x * x - y * y; }
Function current_func = sin_m_cos_func;

void build_surface(Function func, double x0, double x1, double y0, double y1, int steps) {
    pol = polyhedron(2 * steps * steps);
    double dx = (x1 - x0) / steps;
    double dy = (y1 - y0) / steps;

    for (int i = 0; i <= steps; ++i) {
        for (int j = 0; j <= steps; ++j) {
            double x = x0 + i * dx;
            double y = y0 + j * dy;
            double z = func(x, y);
            pol.add_point({ x, y, z });
        }
    }

    pol.apply_view_matr(view_matr);

    for (int i = 0; i < steps; ++i) {
        for (int j = 0; j < steps; ++j) {
            uint idx0 = i * (steps + 1) + j;
            uint idx1 = idx0 + 1;
            uint idx2 = idx0 + (steps + 1);
            uint idx3 = idx2 + 1;

            pol.tie_vertex_to_face(idx0, 2 * (i * steps + j));
            pol.tie_vertex_to_face(idx1, 2 * (i * steps + j));
            pol.tie_vertex_to_face(idx2, 2 * (i * steps + j));

            pol.tie_vertex_to_face(idx1, 2 * (i * steps + j) + 1);
            pol.tie_vertex_to_face(idx3, 2 * (i * steps + j) + 1);
            pol.tie_vertex_to_face(idx2, 2 * (i * steps + j) + 1);
        }
    }

    float kx = 45;
    float ky = 45;
    float kz = 45;
    scalin_matr[0][0] = kx;
    scalin_matr[1][1] = ky;
    scalin_matr[2][2] = kz;
    auto m = general_transformation(pol.centroid(), scalin_matr);
    pol.affine_transformation(m, projection_matr, view_matr);

    double teta = -45 * (M_PI / 180.0);
    change_rotate_mart(teta, 0);
    point p = pol.centroid();
    m = general_transformation(p, rotate_matr);
    pol.affine_transformation(m, projection_matr, view_matr);

    teta = -30 * (M_PI / 180.0);
    change_rotate_mart(teta, 1);
    p = pol.centroid();
    m = general_transformation(p, rotate_matr);
    pol.affine_transformation(m, projection_matr, view_matr);
}

void draw_UI() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 100));
    ImGui::Begin("Instruments", NULL,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse);

    static int choose = 0;
    ImGui::SetCursorPos(ImVec2(10, 25));
    if (ImGui::RadioButton("6", &choose, 6)) {
        change_polyhedron(choose);
    }
    ImGui::SetCursorPos(ImVec2(40, 25));
    if (ImGui::RadioButton("4", &choose, 4)) {
        change_polyhedron(choose);
    }
    ImGui::SetCursorPos(ImVec2(70, 25));
    if (ImGui::RadioButton("20", &choose, 20)) {
        change_polyhedron(choose);
    }
    ImGui::SetCursorPos(ImVec2(25, 50));
    if (ImGui::RadioButton("8", &choose, 8)) {
        change_polyhedron(choose);
    }
    ImGui::SetCursorPos(ImVec2(55, 50));
    if (ImGui::RadioButton("12", &choose, 12)) {
        change_polyhedron(choose);
    }

    ImGui::SetCursorPos(ImVec2(5, 75));
    static bool show_new_content_window = false;
    if (ImGui::Button("NEW CONTENT", ImVec2(100, 20))) {
        show_new_content_window = !show_new_content_window;
    }

    if (show_new_content_window) {
        static int vertex_count = 3;
        ImGui::SetNextWindowPos(ImVec2(0, 100));
        ImGui::SetNextWindowSize(ImVec2(250, 300));
        ImGui::Begin("Solid of Rotation", NULL,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);
        static std::vector<int> x_vec{ 0, 50, 0 };
        static std::vector<int> y_vec{ 0, 0, -100 };

        ImGui::SetCursorPos(ImVec2(0, 25));
        if (ImGui::Button("+ Point", ImVec2(100, 25))) {
            ++vertex_count;
            x_vec.emplace_back();
            y_vec.emplace_back();
            plane_figure.emplace_back();
        }
        ImGui::SetCursorPos(ImVec2(125, 25));
        if (ImGui::Button("- Point", ImVec2(100, 25)) && vertex_count > 0) {
            --vertex_count;
            x_vec.pop_back();
            y_vec.pop_back();
            plane_figure.pop_back();
        }

        for (int i = 0; i < vertex_count; ++i) {
            ImGui::SetCursorPos(ImVec2(0, 60 + 25 * i));
            ImGui::SetNextItemWidth(100);
            std::string sx = "x" + std::to_string(i);
            if (ImGui::InputInt(sx.c_str(), &x_vec[i])) {
                plane_figure[i].x = x_vec[i];
            }
            ImGui::SetNextItemWidth(100);
            ImGui::SetCursorPos(ImVec2(125, 60 + 25 * i));
            std::string sy = "y" + std::to_string(i);
            if (ImGui::InputInt(sy.c_str(), &y_vec[i])) {
                plane_figure[i].y = y_vec[i];
            }
        }

        ImGui::SetCursorPos(ImVec2(0, 60 + 25 * vertex_count));
        ImGui::SetNextItemWidth(60);
        ImGui::LabelText("Axe of Rotation", "");

        ImGui::SetCursorPos(ImVec2(0, 85 + 25 * vertex_count));
        ImGui::SetNextItemWidth(100);
        static std::pair<int, int> p1;
        if (ImGui::InputInt("ax", &p1.first)) {
            axe_of_rotation.first.x = p1.first;
        }
        ImGui::SetNextItemWidth(100);
        ImGui::SetCursorPos(ImVec2(125, 85 + 25 * vertex_count));
        if (ImGui::InputInt("ay", &p1.second)) {
            axe_of_rotation.first.y = p1.second;
        }
        ImGui::SetCursorPos(ImVec2(0, 110 + 25 * vertex_count));
        ImGui::SetNextItemWidth(100);
        static std::pair<int, int> p2 = { 0, 1 };
        if (ImGui::InputInt("bx", &p2.first)) {
            axe_of_rotation.second.x = p2.first;
        }
        ImGui::SetNextItemWidth(100);
        ImGui::SetCursorPos(ImVec2(125, 110 + 25 * vertex_count));
        if (ImGui::InputInt("by", &p2.second)) {
            axe_of_rotation.second.y = p2.second;
        }

        ImGui::SetCursorPos(ImVec2(0, 135 + 25 * vertex_count));
        ImGui::SetNextItemWidth(60);
        ImGui::LabelText("Slide to Build!", "");
        ImGui::SetCursorPos(ImVec2(0, 160 + 25 * vertex_count));
        ImGui::SetNextItemWidth(225);
        if (ImGui::SliderInt("n", &number_of_revolutions, 3, 50)) {
            choose = -1;
            change_polyhedron(choose);
        }

        static int x_0 = -5;
        static int x_1 = 5;
        static int y_0 = -5;
        static int y_1 = 5;
        static int steps = 50;

        ImGui::SetCursorPos(ImVec2(0, 195 + 25 * vertex_count));
        if (ImGui::Button("sinx*cosx")) {
            current_func = sin_m_cos_func;
            build_surface(current_func, x_0, x_1, y_0, y_1, steps);
        }
        ImGui::SetCursorPos(ImVec2(80, 195 + 25 * vertex_count));
        if (ImGui::Button("x+y")) {
            current_func = sin_p_cos_func;
            build_surface(current_func, x_0, x_1, y_0, y_1, steps);
        }
        ImGui::SetCursorPos(ImVec2(120, 195 + 25 * vertex_count));
        if (ImGui::Button("x*y")) {
            current_func = x_m_y_func;
            build_surface(current_func, x_0, x_1, y_0, y_1, steps);
        }
        ImGui::SetCursorPos(ImVec2(0, 225 + 25 * vertex_count));
        if (ImGui::Button("x^2+y^2")) {
            current_func = x2_p_y2_func;
            build_surface(current_func, x_0, x_1, y_0, y_1, steps);
        }
        ImGui::SetCursorPos(ImVec2(70, 225 + 25 * vertex_count));
        if (ImGui::Button("x^2*y&2")) {
            current_func = x2_m_y2_func;
            build_surface(current_func, x_0, x_1, y_0, y_1, steps);
        }
        
        ImGui::SetCursorPos(ImVec2(0, 255 + 25 * vertex_count));
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("x_0", &x_0)) {
            build_surface(current_func, x_0, x_1, y_0, y_1, steps);
        }

        ImGui::SetCursorPos(ImVec2(110, 255 + 25 * vertex_count));
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("x_1", &x_1)) {
            build_surface(current_func, x_0, x_1, y_0, y_1, steps);
        }

        ImGui::SetCursorPos(ImVec2(0, 285 + 25 * vertex_count));
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("y_0", &y_0)) {
            build_surface(current_func, x_0, x_1, y_0, y_1, steps);
        }

        ImGui::SetCursorPos(ImVec2(110, 285 + 25 * vertex_count));
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("y_1", &y_1)) {
            build_surface(current_func, x_0, x_1, y_0, y_1, steps);
        }

        ImGui::SetCursorPos(ImVec2(0, 315 + 25 * vertex_count));
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("st", &steps)) {
            build_surface(current_func, x_0, x_1, y_0, y_1, steps);
        }
        
        ImGui::SetCursorPos(ImVec2(0, 345 + 25 * vertex_count));
        ImGui::SetNextItemWidth(110);
        if (ImGui::Button("Save to obj", ImVec2(100, 25))) {
            if (pol.faces.size() > 0)
                pol.save_to_obj("model.obj");
        }
        ImGui::SetCursorPos(ImVec2(115, 345 + 25 * vertex_count));
        ImGui::SetNextItemWidth(110);
        if (ImGui::Button("Load from obj", ImVec2(100, 25))) {
            pol.load_from_obj("model.obj");
        }
        ImGui::SetCursorPos(ImVec2(0, 375 + 25 * vertex_count));
        ImGui::SetNextItemWidth(110);
        if (ImGui::Button("Load teapot", ImVec2(100, 25))) {
            pol.load_from_obj("utah_teapot_lowpoly.obj");
        }

        ImGui::End();
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
        pol.affine_transformation(offset_matr, projection_matr, view_matr);
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
    ImGui::SetCursorPos(ImVec2(360, 75));
    static bool by_center_rot = 0;
    ImGui::Checkbox("rotate by center", &by_center_rot);
    ImGui::SetCursorPos(ImVec2(480, 27));
    if (ImGui::Button("Rotate", ImVec2(100, 50))) {
        point p = { 300, 300, 300 };
        if (by_center_rot)
            p = pol.centroid();
        auto m = general_transformation(p, rotate_matr);
        pol.affine_transformation(m, projection_matr, view_matr);
    }

    ImGui::SetCursorPos(ImVec2(590, 27));
    ImGui::SetNextItemWidth(100);
    static float kx = 1;
    ImGui::InputFloat("kx", &kx);
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(590, 50));
    static float ky = 1;
    ImGui::InputFloat("ky", &ky);
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(590, 73));
    static float kz = 1;
    ImGui::InputFloat("kz", &kz);
    ImGui::SetCursorPos(ImVec2(720, 27));
    static bool by_center_sc = 0;
    ImGui::Checkbox("by center", &by_center_sc);
    ImGui::SetCursorPos(ImVec2(815, 27));
    if (ImGui::Button("Scale", ImVec2(100, 50))) {
        scalin_matr[0][0] = kx;
        scalin_matr[1][1] = ky;
        scalin_matr[2][2] = kz;
        if (by_center_sc) {
            auto m = general_transformation(pol.centroid(), scalin_matr);
            pol.affine_transformation(m, projection_matr, view_matr);
        }
        else
            pol.affine_transformation(scalin_matr, projection_matr, view_matr);
    }

    static int refl_index = -1;
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(925, 27));
    ImGui::RadioButton("Oyz", &refl_index, 0);
    ImGui::SetCursorPos(ImVec2(925, 50));
    ImGui::RadioButton("Oxz", &refl_index, 1);
    ImGui::SetCursorPos(ImVec2(925, 73));
    ImGui::RadioButton("Oxy", &refl_index, 2);
    ImGui::SetCursorPos(ImVec2(980, 27));
    if (ImGui::Button("Reflect", ImVec2(100, 50))) {
        scalin_matr[0][0] = refl_index == 0 ? -1 : 1;
        scalin_matr[1][1] = refl_index == 1 ? -1 : 1;
        scalin_matr[2][2] = refl_index == 2 ? -1 : 1;
        pol.affine_transformation(scalin_matr, projection_matr, view_matr);
    }

    static float alphal = 0;
    static double tetal = 0;
    static float p1x = 0;
    static float p1y = -500;
    static float p1z = 0;
    static float p2x = 100;
    static float p2y = 500;
    static float p2z = 0;

    ImGui::SetCursorPos(ImVec2(1100, 27));
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputFloat("p1x", &p1x))
    {
        tetal = -alphal * (M_PI / 180.0);
        change_rotate_around_line(tetal, { p1x, p1y, p1z }, { p2x, p2y, p2z });
    }
    ImGui::SetNextItemWidth(80);
    ImGui::SetCursorPos(ImVec2(1100, 50));
    if (ImGui::InputFloat("p1y", &p1y))
    {
        tetal = -alphal * (M_PI / 180.0);
        change_rotate_around_line(tetal, { p1x, p1y, p1z }, { p2x, p2y, p2z });
    }
    ImGui::SetNextItemWidth(80);
    ImGui::SetCursorPos(ImVec2(1100, 73));
    if (ImGui::InputFloat("p1z", &p1z))
    {
        tetal = -alphal * (M_PI / 180.0);
        change_rotate_around_line(tetal, { p1x, p1y, p1z }, { p2x, p2y, p2z });
    }

    ImGui::SetCursorPos(ImVec2(1210, 27));
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputFloat("p2x", &p2x))
    {
        tetal = -alphal * (M_PI / 180.0);
        change_rotate_around_line(tetal, { p1x, p1y, p1z }, { p2x, p2y, p2z });
    }
    ImGui::SetNextItemWidth(80);
    ImGui::SetCursorPos(ImVec2(1210, 50));
    if (ImGui::InputFloat("p2y", &p2y))
    {
        tetal = -alphal * (M_PI / 180.0);
        change_rotate_around_line(tetal, { p1x, p1y, p1z }, { p2x, p2y, p2z });
    }
    ImGui::SetNextItemWidth(80);
    ImGui::SetCursorPos(ImVec2(1210, 73));
    if (ImGui::InputFloat("p2z", &p2z))
    {
        tetal = -alphal * (M_PI / 180.0);
        change_rotate_around_line(tetal, { p1x, p1y, p1z }, { p2x, p2y, p2z });
    }

    ImGui::SetCursorPos(ImVec2(1320, 27));
    if (ImGui::Button("DrawLine", ImVec2(100, 50))) {
        isDrawingLine = !isDrawingLine;
    }

    if (isDrawingLine)
    {
        DrawLineWu({ view_matr[3][0] + p1x, view_matr[3][1] + p1y, p1z }, { view_matr[3][0] + p2x, view_matr[3][1] + p2y, p2z }, ImVec4(1.0, 0.0, 0.0, 1.0));
    }

    ImGui::SetCursorPos(ImVec2(1430, 27));
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputFloat("la", &alphal)) {
        tetal = -alphal * (M_PI / 180.0);
        change_rotate_around_line(tetal, { p1x, p1y, p1z }, { p2x, p2y, p2z });
    }

    ImGui::SetCursorPos(ImVec2(1540, 27));
    if (ImGui::Button("RotateAround", ImVec2(100, 50))) {
        auto m = general_transformation({ p1x, p1y, p1z }, rotate_matr_line);
        pol.affine_transformation(m, projection_matr, view_matr);
    }

    static float c = viewport->Size.x;
    auto perspective_matrix = create_perspective_matrix(c);
    ImGui::SetCursorPos(ImVec2(1660, 50));
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputFloat("c", &c)) {
        if (isPerspec)
        {
            perspective_matrix = create_perspective_matrix(c);
            projection_matr = perspective_matrix;
            auto res_matr = matr_mult(projection_matr, view_matr);
            pol.apply_view_matr(res_matr);
        }
    }

    ImGui::SetCursorPos(ImVec2(1660, 27));
    if (ImGui::Button("Perspective")) {
        if (!isPerspec) {
            perspective_matrix = create_perspective_matrix(c);
            projection_matr = perspective_matrix;
            auto res_matr = matr_mult(projection_matr, view_matr);
            pol.apply_view_matr(res_matr);
            isPerspec = true;
        }
        else {
            projection_matr = base_proj_matr;
            pol.apply_view_matr(view_matr);
            isPerspec = false;
        }
    }

    static int psig = 0;
    static int phig = 0;
    static float psi = 0;
    static float phi = 0;
    auto axo_matrix = create_axo_matrix(psi, phi);
    ImGui::SetCursorPos(ImVec2(1770, 50));
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("psi", &psig)) {
        psi = psig * M_PI / 180;
        axo_matrix = create_axo_matrix(psi, phi);
        if (isAxo) {
            projection_matr = axo_matrix;
            auto res_matr = matr_mult(projection_matr, view_matr);
            pol.apply_view_matr(res_matr);
        }
    }

    ImGui::SetCursorPos(ImVec2(1770, 73));
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("phi", &phig)) {
        phi = phig * M_PI / 180;
        axo_matrix = create_axo_matrix(psi, phi);
        if (isAxo) {
            projection_matr = axo_matrix;
            auto res_matr = matr_mult(projection_matr, view_matr);
            pol.apply_view_matr(res_matr);
        }
    }

    ImGui::SetCursorPos(ImVec2(1770, 27));
    ImGui::SetNextItemWidth(80);
    if (ImGui::Button("Axonometric")) {
        if (!isAxo) {
            axo_matrix = create_axo_matrix(psi, phi);
            projection_matr = axo_matrix;
            auto res_matr = matr_mult(projection_matr, view_matr);
            pol.apply_view_matr(res_matr);
            isAxo = true;
        }
        else {
            projection_matr = base_proj_matr;
            pol.apply_view_matr(view_matr);
            isAxo = false;
        }
    }

    ImGui::End();
}
int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(1900, 1000, "3D", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    initialize_matrixes();
    glEnable(GL_DEPTH_TEST);

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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
