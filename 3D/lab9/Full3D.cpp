#include <iostream>
#include <vector>
#include <random>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <string>

bool use_z_buffer = true;
bool use_gouraud_shading = true;
bool use_phong_shading = false;

const int WIDTHSZ = 1900;
const int HEIGHTSZ = 1000;

const int CENTERX = WIDTHSZ / 2;
const int CENTERY = HEIGHTSZ / 2;

using uint = unsigned int;

class point {
    std::vector<double> coords;

public:
    double& x;
    double& y;
    double& z;
    double& w;
    point(double x = 0, double y = 0, double z = 0, double w = 1):
        coords{ x, y, z, w }, x(coords[0]), y(coords[1]), z(coords[2]), w(coords[3]) {}

    point(const point& p):
        coords(p.coords), x(coords[0]), y(coords[1]), z(coords[2]), w(coords[3]) {}

    point& operator=(const point& p) {
        coords = p.coords;
        return *this;
    }

    point operator-(const point& p) const {
        return point(x - p.x, y - p.y, z - p.z);
    }

    point operator*(double scalar) const {
        return point(x * scalar, y * scalar, z * scalar);
    }

    point& operator*=(double scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    void normalize() {
        if (w == 0) return;
        x /= w;
        y /= w;
        z /= w;
        w = 1;
    }

    void affine_transformation(std::vector<std::vector<double>>& m, bool is_normal = false) {
        std::vector<double> new_coor(4);
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k)
                new_coor[j] += coords[k] * m[k][j];
        coords = new_coor;
        if (!is_normal)
            normalize();
    }
};

void DrawPoint(int x, int y, float intencity) {
    x += CENTERX;
    y += CENTERY + 100;
    ImColor c(0, 0, 0, 255);
    c.Value.w *= intencity;
    ImGui::GetForegroundDrawList()->
        AddRectFilled(ImVec2(x, y), ImVec2(x + 1, y + 1), c);
}


ImColor screen[WIDTHSZ * HEIGHTSZ];
double z_buffer[WIDTHSZ * HEIGHTSZ];
bool redraw[WIDTHSZ * HEIGHTSZ];
int min_changed_x = WIDTHSZ;
int min_changed_y = HEIGHTSZ;
int max_changed_x = 0;
int max_changed_y = 0;

void set_pixel(const point& p, const ImColor& c) {
    int x = static_cast<int>(p.x) + CENTERX;
    int y = static_cast<int>(p.y) + CENTERY;
    int z = static_cast<int>(p.z);
    if (x < 0 || WIDTHSZ <= x || y < 0 || HEIGHTSZ <= y) return;
    int index = x * HEIGHTSZ + y;
    if (z > z_buffer[index]) {
        z_buffer[index] = z;
        if (screen[index] != c) {
            min_changed_x = std::min(min_changed_x, x);
            min_changed_y = std::min(min_changed_y, y);
            max_changed_x = std::max(max_changed_x, x);
            max_changed_y = std::max(max_changed_y, y);
            redraw[index] = true;
            screen[index] = c;
        }
    }
}

std::vector<std::vector<double>> look_at(const point& eye, const point& center, const point& up) {

    std::vector<double> f = { center.x - eye.x, center.y - eye.y, center.z - eye.z };
    double f_length = sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    for (double& val : f)
        val /= f_length;

    std::vector<double> u = { up.x, up.y, up.z };
    double u_length = sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    for (double& val : u)
        val /= u_length;

    std::vector<double> s = { f[1] * u[2] - f[2] * u[1], f[2] * u[0] - f[0] * u[2], f[0] * u[1] - f[1] * u[0] };
    double s_length = sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
    for (double& val : s)
        val /= s_length;

    u = { s[1] * f[2] - s[2] * f[1], s[2] * f[0] - s[0] * f[2], s[0] * f[1] - s[1] * f[0] };

    std::vector<std::vector<double>> result = {
        { s[0], s[1], s[2], -s[0] * eye.x - s[1] * eye.y - s[2] * eye.z },
        { u[0], u[1], u[2], -u[0] * eye.x - u[1] * eye.y - u[2] * eye.z },
        { -f[0], -f[1], -f[2], f[0] * eye.x + f[1] * eye.y + f[2] * eye.z },
        { 0, 0, 0, 1 }
    };

    return result;
}

double scale_factor = 0.0001;

double camera_x = 0 * scale_factor, camera_y = 0 * scale_factor, camera_z = 10 * scale_factor;
double camera_angle_x = 0, camera_angle_y = 0, camera_angle_z = 0;
double camera_rotation_speed = 0.02;
double camera_move_speed = 1 * scale_factor;

std::vector<std::vector<double>> create_camera_view()
{
    point eye(camera_x, camera_y, camera_z);
    point center(
        camera_x + cos(camera_angle_y) * cos(camera_angle_x)
        , camera_y + sin(camera_angle_x)
        , camera_z + sin(camera_angle_y) * cos(camera_angle_x));
    point up(0, 1, 0);
    auto view = look_at(eye, center, up);

    return view;
}

void reset_z_buffer_related_variables() {
    std::fill(screen, screen + WIDTHSZ * HEIGHTSZ, ImColor(255, 255, 255));
    std::fill(z_buffer, z_buffer + WIDTHSZ * HEIGHTSZ, -10000000000000);
    min_changed_x = WIDTHSZ;
    min_changed_y = HEIGHTSZ;
    max_changed_x = 0;
    max_changed_y = 0;
}

void DrawScreen() {
    for (int x = min_changed_x; x < max_changed_x; ++x)
        for (int y = min_changed_y; y < max_changed_y; ++y) {
            int index = x * HEIGHTSZ + y;
            if (redraw[index]) {
                ImGui::GetForegroundDrawList()->
                    AddRectFilled(ImVec2(x, y + 100), ImVec2(x + 1, y + 100 + 1), screen[index]);
                redraw[index] = false;
            }
        }
}

unsigned char* LoadTextureFromFile(const char* filepath, int& width, int& height, int& channels) {
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
    }
    return data;
}

int texWidth, texHeight, texChannels;
unsigned char* textureData = LoadTextureFromFile("bus2.png", texWidth, texHeight, texChannels);

void DrawLine(const point& p0, const point& p1,
    const point& pt0, const point& pt1) {
    int x0 = p0.x, y = p0.y, z0 = p0.z;
    int x1 = p1.x, z1 = p1.z;

    int dx = abs(x1 - x0), dz = abs(z1 - z0);
    int xd = (x0 < x1) ? +1 : -1;
    int zd = (z0 < z1) ? +1 : -1;
    int maxDelta = std::max(dx, dz);

    int errX = maxDelta / 2, errZ = maxDelta / 2;

    double u0 = pt0.x;
    double v0 = pt0.y;
    double u1 = pt1.x;
    double v1 = pt1.y;

    double du = (u1 - u0) / maxDelta;
    double dv = (v1 - v0) / maxDelta;

    double u = u0, v = v0;

    for (int i = 0; i <= maxDelta; ++i) {
        int texX = static_cast<int>(u * texWidth) % texWidth;
        int texY = static_cast<int>(v * texHeight) % texHeight;
        texX = std::max(0, texX);
        texY = std::max(0, texY);

        int texIndex = (texY * texWidth + texX) * texChannels;
        ImColor color(textureData[texIndex] / 255.0f,
            textureData[texIndex + 1] / 255.0f,
            textureData[texIndex + 2] / 255.0f);

        set_pixel({ static_cast<double>(x0)
            , static_cast<double>(y)
            , static_cast<double>(z0) }
        , color);

        u += du;
        v += dv;

        errX -= dx;
        errZ -= dz;

        if (errX < 0) {
            x0 += xd;
            errX += maxDelta;
        }
        if (errZ < 0) {
            z0 += zd;
            errZ += maxDelta;
        }
    }
}

float InterpolateIntenX(float x, const point& p1, const point& p2,
    float inten_p1, float inten_p2) {
    float t = (x - p1.x) / (p2.x - p1.x);
    return inten_p1 + t * (inten_p2 - inten_p1);
}

void DrawLineBresenham(const point& p0, const point& p1, const ImColor& obj_color,
    float inten_p0, float inten_p1) {
    int x0 = p0.x;
    int y0 = p0.y;
    int z0 = p0.z;
    int x1 = p1.x;
    int y1 = p1.y;
    int z1 = p1.z;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int dz = abs(z1 - z0);

    int xd = (x0 < x1) ? 1 : -1;
    int yd = (y0 < y1) ? 1 : -1;
    int zd = (z0 < z1) ? 1 : -1;

    int maxDelta = std::max({ dx, dy, dz });

    int errX = maxDelta / 2;
    int errY = maxDelta / 2;
    int errZ = maxDelta / 2;

    for (int i = 0; i <= maxDelta; ++i) {
        float inten = InterpolateIntenX(x0, p0, p1, inten_p0, inten_p1);
        set_pixel({ (double)x0, (double)y0, (double)z0 },
            { obj_color.Value.x * inten, obj_color.Value.y * inten, obj_color.Value.z * inten });

        errX -= dx;
        errY -= dy;
        errZ -= dz;

        if (errX < 0) {
            x0 += xd;
            errX += maxDelta;
        }
        if (errY < 0) {
            y0 += yd;
            errY += maxDelta;
        }
        if (errZ < 0) {
            z0 += zd;
            errZ += maxDelta;
        }
    }
}


void DrawLineWu(point p0, point p1) {
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
        DrawPoint(x0, y0, 1);
        float y = y0 + gradient;
        for (int x = x0 + 1; x <= x1 - 1; ++x) {
            DrawPoint(x, (int)y, 1 - (y - (int)y));
            DrawPoint(x, (int)y + 1, y - (int)y);
            y += gradient;
        }
        DrawPoint(x1, y1, 1);
    }
    else {
        gradient = (float)dx / dy;
        if (y0 > y1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        DrawPoint(x0, y0, 1);
        float x = x0 + gradient;
        for (int y = y0 + 1; y <= y1 - 1; ++y) {
            DrawPoint((int)x, y, 1 - (x - (int)x));
            DrawPoint((int)x + 1, y, x - (int)x);
            x += gradient;
        }
        DrawPoint(x1, y1, 1);
    }
}

double InterpoalteFactor(double y, const point& p1, const point& p2) {
    return (abs(p2.y - p1.y) < 0.01) ? 0.0 : (y - p1.y) / (p2.y - p1.y);
}

double InterpoalteFactorX(double x, const point& p1, const point& p2) {
    return (abs(p2.x - p1.x) < 0.01) ? 0.0 : (x - p1.x) / (p2.x - p1.x);
}

point InterpolateVertexY(double factor, const point& p1, const point& p2) {
    return { p1.x + factor * (p2.x - p1.x), p1.y + factor * (p2.y - p1.y), p1.z + factor * (p2.z - p1.z) };
}

void DrawTriangle(const point& p1, const point& p2, const point& p3
    , const point& pt1, const point& pt2, const point& pt3) {
    std::vector<point> vertices{ p1, p2, p3 };
    std::vector<point> text_vertices{ pt1, pt2, pt3 };
    if (vertices[1].y < vertices[0].y) {
        std::swap(vertices[0], vertices[1]);
        std::swap(text_vertices[0], text_vertices[1]);
    }
    if (vertices[2].y < vertices[0].y) {
        std::swap(vertices[0], vertices[2]);
        std::swap(text_vertices[0], text_vertices[2]);
    }
    if (vertices[2].y < vertices[1].y) {
        std::swap(vertices[1], vertices[2]);
        std::swap(text_vertices[1], text_vertices[2]);
    }

    const point& top = vertices[0];
    const point& mid = vertices[1];
    const point& bot = vertices[2];

    const point& text_top = text_vertices[0];
    const point& text_mid = text_vertices[1];
    const point& text_bot = text_vertices[2];

    for (double y = top.y; y <= mid.y - 0.001; ++y) {
        double left_factor = InterpoalteFactor(y, top, mid);
        double right_factor = InterpoalteFactor(y, top, bot);

        point left = InterpolateVertexY(left_factor, top, mid);
        point right = InterpolateVertexY(right_factor, top, bot);

        point text_left = InterpolateVertexY(left_factor, text_top, text_mid);
        point text_right = InterpolateVertexY(right_factor, text_top, text_bot);

        DrawLine(left, right, text_left, text_right);
    }

    for (double y = mid.y; y <= bot.y - 0.001; ++y) {
        double left_factor = InterpoalteFactor(y, mid, bot);
        double right_factor = InterpoalteFactor(y, top, bot);

        point left = InterpolateVertexY(left_factor, mid, bot);
        point right = InterpolateVertexY(right_factor, top, bot);

        point text_left = InterpolateVertexY(left_factor, text_mid, text_bot);
        point text_right = InterpolateVertexY(right_factor, text_top, text_bot);

        DrawLine(left, right, text_left, text_right);
    }
}

std::pair<point, float> InterpolateVertexY(float y, const point& p1, const point& p2,
    float inten_p1, float inten_p2) {
    float t = (y - p1.y) / (p2.y - p1.y);
    return std::make_pair(point(p1.x + t * (p2.x - p1.x), y, p1.z + t * (p2.z - p1.z)),
        inten_p1 + t * (inten_p2 - inten_p1));
}

void DrawLitTriangle(const point& p1, const point& p2, const point& p3,
    const ImColor& obj_color, std::vector<double>& intensities) {
    std::vector<point> vertices{ p1, p2, p3 };
    if (vertices[1].y < vertices[0].y) {
        std::swap(vertices[0], vertices[1]);
        std::swap(intensities[0], intensities[1]);
    }
    if (vertices[2].y < vertices[0].y) {
        std::swap(vertices[0], vertices[2]);
        std::swap(intensities[0], intensities[2]);
    }
    if (vertices[2].y < vertices[1].y) {
        std::swap(vertices[1], vertices[2]);
        std::swap(intensities[1], intensities[2]);
    }

    const point& top = vertices[0];
    const point& mid = vertices[1];
    const point& bot = vertices[2];

    for (float y = top.y; y < mid.y - 0.01; ++y) {
        auto left = InterpolateVertexY(y, top, mid, intensities[0], intensities[1]);
        auto right = InterpolateVertexY(y, top, bot, intensities[0], intensities[2]);
        DrawLineBresenham(left.first, right.first, obj_color, left.second, right.second);
    }

    for (float y = mid.y; y < bot.y - 0.01; ++y) {
        auto left = InterpolateVertexY(y, mid, bot, intensities[1], intensities[2]);
        auto right = InterpolateVertexY(y, top, bot, intensities[0], intensities[2]);
        DrawLineBresenham(left.first, right.first, obj_color, left.second, right.second);
    }
}

std::vector<std::vector<double>> offset_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> rotate_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> scalin_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> view_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> projection_matr(4, std::vector<double>(4));
std::vector<std::vector<double>> base_proj_matr(4, std::vector<double>(4));
point base_view_vec = { 0, 0, 1 };
point cur_view_vec = base_view_vec;

void initialize_matrixes() {
    offset_matr[0][0] = offset_matr[1][1] = offset_matr[2][2] = offset_matr[3][3] = 1;
    scalin_matr[0][0] = scalin_matr[1][1] = scalin_matr[2][2] = scalin_matr[3][3] = 1;
    projection_matr[0][0] = projection_matr[1][1] = projection_matr[2][2] = projection_matr[3][3] = 1;
    base_proj_matr = projection_matr;
    view_matr = create_camera_view();
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

double determinant3x3(const std::vector<std::vector<double>>& matrix) {
    return
        matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
        matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
        matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
}

std::vector<std::vector<double>> transposeMatrix(const std::vector<std::vector<double>>& matrix) {
    int n = matrix.size();
    int m = matrix[0].size();
    std::vector<std::vector<double>> transposed(m, std::vector<double>(n, 0.0));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            transposed[j][i] = matrix[i][j];
        }
    }

    return transposed;
}

std::vector<std::vector<double>> inverse3x3(const std::vector<std::vector<double>>& matrix) {
    double inv_det = 1.0 / determinant3x3(matrix);
    std::vector<std::vector<double>> inv_matrix(3, std::vector<double>(3, 0.0));

    inv_matrix[0][0] = (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) * inv_det;
    inv_matrix[0][1] = (matrix[0][2] * matrix[2][1] - matrix[0][1] * matrix[2][2]) * inv_det;
    inv_matrix[0][2] = (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) * inv_det;

    inv_matrix[1][0] = (matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2]) * inv_det;
    inv_matrix[1][1] = (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]) * inv_det;
    inv_matrix[1][2] = (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]) * inv_det;

    inv_matrix[2][0] = (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]) * inv_det;
    inv_matrix[2][1] = (matrix[0][1] * matrix[2][0] - matrix[0][0] * matrix[2][1]) * inv_det;
    inv_matrix[2][2] = (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]) * inv_det;

    return inv_matrix;
}

std::vector<std::vector<double>> transposedInverseMatrix(
    const std::vector<std::vector<double>>& matrix) {
    auto inv_matrix = inverse3x3({
        {matrix[0][0], matrix[0][1], matrix[0][2]},
        {matrix[1][0], matrix[1][1], matrix[1][2]},
        {matrix[2][0], matrix[2][1], matrix[2][2]}
        });
    inv_matrix = transposeMatrix(inv_matrix);

    std::vector<std::vector<double>> norm_matrix(4, std::vector<double>(4, 0.0));
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j)
            norm_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            norm_matrix[i][j] = inv_matrix[i][j];
    }

    return norm_matrix;
}

point light_pos = { 0, 0, 1 };

double dot_product(const point& v1, const point& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

void normalize(point& vec) {
    double len = std::sqrt(dot_product(vec, vec));
    if (len > 0) {
        vec.x /= len;
        vec.y /= len;
        vec.z /= len;
    }
}

ImColor CalcPhongColor(const point& p, const point& normal, const ImColor& obj_color) {
    point light_dir = light_pos - p;
    normalize(light_dir);

    double ambient = 0.1;
    double diffuse = std::max(dot_product(normal, light_dir), 0.0);
    double specular = 0.0;

    point view_dir = { -p.x, -p.y, -p.z };
    normalize(view_dir);

    if (diffuse > 0) {
        point reflect_dir = normal;
        reflect_dir.x = 2 * dot_product(normal, light_dir) * normal.x - light_dir.x;
        reflect_dir.y = 2 * dot_product(normal, light_dir) * normal.y - light_dir.y;
        reflect_dir.z = 2 * dot_product(normal, light_dir) * normal.z - light_dir.z;
        normalize(reflect_dir);
        specular = std::pow(std::max(dot_product(view_dir, reflect_dir), 0.0), 16);
    }

    double intensity = ambient + 0.7 * diffuse + 0.2 * specular;
    intensity = std::min(intensity, 1.0);

    if (intensity > 0.8) 
        intensity = 1.0;
    else if (intensity > 0.4)
        intensity = 0.6;
    else
        intensity = 0.2;
    
    int r = static_cast<int>(obj_color.Value.x * 255 * intensity);
    int g = static_cast<int>(obj_color.Value.y * 255 * intensity);
    int b = static_cast<int>(obj_color.Value.z * 255 * intensity);
    return ImColor(r, g, b);
}

void DrawPhongLine(const point& p0, const point& p1, const point& n0, const point& n1, const ImColor& obj_color) {
    int x0 = p0.x;
    int y0 = p0.y;
    int z0 = p0.z;
    int x1 = p1.x;
    int y1 = p1.y;
    int z1 = p1.z;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int dz = abs(z1 - z0);

    int xd = (x0 < x1) ? 1 : -1;
    int yd = (y0 < y1) ? 1 : -1;
    int zd = (z0 < z1) ? 1 : -1;

    int maxDelta = std::max({ dx, dy, dz });

    int errX = maxDelta / 2;
    int errY = maxDelta / 2;
    int errZ = maxDelta / 2;

    for (int i = 0; i <= maxDelta; ++i) {
        double t = InterpoalteFactorX(x0, p0, p1);
        point interp_pos = InterpolateVertexY(t, p0, p1);
        point normal = InterpolateVertexY(t, n0, n1);
        normalize(normal);

        ImColor color = CalcPhongColor(interp_pos, normal, obj_color);
        set_pixel({ (double)x0, (double)y0, (double)z0 }, color);

        errX -= dx;
        errY -= dy;
        errZ -= dz;

        if (errX < 0) {
            x0 += xd;
            errX += maxDelta;
        }
        if (errY < 0) {
            y0 += yd;
            errY += maxDelta;
        }
        if (errZ < 0) {
            z0 += zd;
            errZ += maxDelta;
        }
    }
}

void DrawPhongTriangle(const point& p1, const point& p2, const point& p3,
    const point& n1, const point& n2, const point& n3,
    const ImColor& color) {
    std::vector<point> vertices{ p1, p2, p3 };
    std::vector<point> normals{ n1, n2, n3 };

    if (vertices[1].y < vertices[0].y) {
        std::swap(vertices[0], vertices[1]);
        std::swap(normals[0], normals[1]);
    }
    if (vertices[2].y < vertices[0].y) {
        std::swap(vertices[0], vertices[2]);
        std::swap(normals[0], normals[2]);
    }
    if (vertices[2].y < vertices[1].y) {
        std::swap(vertices[1], vertices[2]);
        std::swap(normals[1], normals[2]);
    }

    const point& top = vertices[0];
    const point& mid = vertices[1];
    const point& bot = vertices[2];

    const point& norm_top = normals[0];
    const point& norm_mid = normals[1];
    const point& norm_bot = normals[2];

    for (double y = top.y; y < mid.y - 0.01; ++y) {
        double left_factor = InterpoalteFactor(y, top, mid);
        double right_factor = InterpoalteFactor(y, top, bot);

        point left = InterpolateVertexY(left_factor, top, mid);
        point right = InterpolateVertexY(right_factor, top, bot);

        point norm_left = InterpolateVertexY(left_factor, norm_top, norm_mid);
        point norm_right = InterpolateVertexY(right_factor, norm_top, norm_bot);

        DrawPhongLine(left, right, norm_left, norm_right, color);
    }

    for (double y = mid.y; y < bot.y - 0.01; ++y) {
        double left_factor = InterpoalteFactor(y, mid, bot);
        double right_factor = InterpoalteFactor(y, top, bot);

        point left = InterpolateVertexY(left_factor, mid, bot);
        point right = InterpolateVertexY(right_factor, top, bot);

        point norm_left = InterpolateVertexY(left_factor, norm_mid, norm_bot);
        point norm_right = InterpolateVertexY(right_factor, norm_top, norm_bot);

        DrawPhongLine(left, right, norm_left, norm_right, color);
    }
}



class polyhedron {
    struct polygon;
    std::vector<polygon> faces;
    std::vector<point> vertices;
    std::vector<point> text_vertices;
    std::vector<point> view_vertices;
    std::vector<point> vert_normals;

    static point cross_product(const point& v1, const point& v2) {
        return { v1.y * v2.z - v1.z * v2.y
            , v1.z * v2.x - v1.x * v2.z
            , v1.x * v2.y - v1.y * v2.x
        };
    }

    static double dot_product(const point& v1, const point& v2) {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    static void normalize(point& vec) {
        double len = std::sqrt(dot_product(vec, vec));
        if (len > 0) {
            vec.x /= len;
            vec.y /= len;
            vec.z /= len;
        }
    }

    struct polygon {
        std::vector<uint> vert_indices;
        std::vector<uint> text_indices;
        std::vector<uint> norm_indices;
        point normal;
        size_t size() const { return vert_indices.size(); }
        void push_back_vert_index(uint v_ind) { vert_indices.push_back(v_ind); }
        void push_back_text_index(uint t_ind) { text_indices.push_back(t_ind); }
        void push_back_norm_index(uint n_ind) { norm_indices.push_back(n_ind); }
    };

public:
    ImColor color = { 255, 255, 255 };

    polyhedron() {}

    void apply_view_matr(std::vector<std::vector<double>>& view_m) {
        view_vertices.assign(vertices.begin(), vertices.end());
        for (auto& view_vertex : view_vertices)
            view_vertex.affine_transformation(view_m);
        calc_face_normals();
    }

    void affine_transformation(std::vector<std::vector<double>>& m
        , std::vector<std::vector<double>>& projection
        , std::vector<std::vector<double>>& view) {
        for (auto& vertex : vertices)
            vertex.affine_transformation(m);
        std::vector<std::vector<double>> normal_mat = transposedInverseMatrix(m);
        for (auto& vert_normal : vert_normals) {
            vert_normal.affine_transformation(normal_mat, true);
            vert_normal *= 10;
            normalize(vert_normal);
        }
        auto proj_view = matr_mult(projection, view);
        apply_view_matr(proj_view);
    }

    std::vector<double> calc_intensity_lambert(const polygon& face) const {
        std::vector<double> res;
        for (int i = 0; i < 3; ++i) {
            point light_dir = light_pos - view_vertices[face.vert_indices[i]];
            normalize(light_dir);
            res.push_back(std::max(dot_product(vert_normals[face.norm_indices[i]],
                light_dir), 0.0));
        }
        return res;
    }

    void draw() const {
        for (const polygon& face : faces) {
            if (dot_product(face.normal, cur_view_vec) <= 0) continue;
            auto& v = face.vert_indices;
            auto& t = face.text_indices;
            auto& n = face.norm_indices;
            if (use_z_buffer) {
                if (use_gouraud_shading) {
                    std::vector<double> inten = calc_intensity_lambert(face);
                    DrawLitTriangle(view_vertices[v[0]]
                        , view_vertices[v[1]]
                        , view_vertices[v[2]]
                        , color
                        , inten);
                }
                else if (use_phong_shading) {
                    DrawPhongTriangle(view_vertices[v[0]], view_vertices[v[1]], view_vertices[v[2]],
                        vert_normals[n[0]], vert_normals[n[1]], vert_normals[n[2]],
                        color);
                }
                else {
                    DrawTriangle(
                        view_vertices[v[0]], view_vertices[v[1]], view_vertices[v[2]],
                        text_vertices[t[0]], text_vertices[t[1]], text_vertices[t[2]]);
                }
            }
            else {
                DrawLineWu(view_vertices[v[0]], view_vertices[v[1]]);
                DrawLineWu(view_vertices[v[1]], view_vertices[v[2]]);
                DrawLineWu(view_vertices[v[2]], view_vertices[v[0]]);
            }
        }
    }

    void add_text_vertex(double x, double y) {
        text_vertices.emplace_back(x, y);
    }

    uint add_point(double x, double y, double z) {
        vertices.emplace_back(x, y, z);
        return vertices.size() - 1;
    }

    void add_vert_normal(double x, double y, double z) {
        vert_normals.emplace_back(x, y, z);
    }

    void tie_vertex_to_face(uint v_ind, uint f_ind) {
        pol.faces[f_ind].push_back_vert_index(v_ind);
    }

    void tie_text_vertex_to_face(uint t_ind, uint f_ind) {
        pol.faces[f_ind].push_back_text_index(t_ind);
    }

    void tie_vert_normal_to_face(uint n_ind, uint f_ind) {
        pol.faces[f_ind].push_back_norm_index(n_ind);
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
        faces.clear();
        vertices.clear();
        view_vertices.clear();
        vert_normals.clear();
    }

    void triangulate_faces() {
        std::vector<polygon> triangled_faces;
        for (polygon& face : faces) {
            for (int j = 1; j < face.size() - 1; ++j) {
                polygon new_face;
                new_face.push_back_vert_index(face.vert_indices[0]);
                new_face.push_back_vert_index(face.vert_indices[j]);
                new_face.push_back_vert_index(face.vert_indices[j + 1]);
                new_face.push_back_text_index(face.text_indices[0]);
                new_face.push_back_text_index(face.text_indices[j]);
                new_face.push_back_text_index(face.text_indices[j + 1]);
                new_face.push_back_norm_index(face.norm_indices[0]);
                new_face.push_back_norm_index(face.norm_indices[j]);
                new_face.push_back_norm_index(face.norm_indices[j + 1]);
                new_face.normal = face.normal;
                triangled_faces.push_back(new_face);
            }
        }
        faces = triangled_faces;
    }

    void calc_face_normals() {
        for (polygon& face : faces) {
            point& vert0 = view_vertices[face.vert_indices[0]];
            point& vert1 = view_vertices[face.vert_indices[1]];
            point& vert2 = view_vertices[face.vert_indices[2]];

            point vec1 = { vert1.x - vert0.x, vert1.y - vert0.y, vert1.z - vert0.z };
            point vec2 = { vert2.x - vert0.x, vert2.y - vert0.y, vert2.z - vert0.z };

            point cr_pr = cross_product(vec1, vec2);
            normalize(cr_pr);
            face.normal = cr_pr;
        }
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
                double x, y, z;
                iss >> x >> y >> z;
                add_point(x, y, z);
            }
            else if (type == "vn") {
                float x, y, z;
                iss >> x >> y >> z;
                add_vert_normal(x, y, z);
            }
            else if (type == "vt") {
                double x, y;
                iss >> x >> y;
                add_text_vertex(x, y);
            }
            else if (type == "f")
                break;
        }

        apply_view_matr(view_matr);

        do {
            std::istringstream iss(line);
            std::string type;
            iss >> type;
            if (type != "f")
                continue;
            int vertex_ind, norm_ind, tex_coord_ind;
            faces.push_back(polygon());
            while (iss >> vertex_ind) {
                tie_vertex_to_face(--vertex_ind, face_index);

                char ch1 = iss.peek();
                if (ch1 == '/') {
                    iss.ignore();
                    ch1 = iss.peek();
                    if (ch1 == '/') {
                        iss.ignore();
                        iss >> norm_ind;
                        tie_vert_normal_to_face(--norm_ind, face_index);
                    }
                    else if (isdigit(ch1)) {
                        iss >> tex_coord_ind;
                        tie_text_vertex_to_face(--tex_coord_ind, face_index);
                        ch1 = iss.peek();
                        if (ch1 == '/') {
                            iss.ignore();
                            iss >> norm_ind;
                            tie_vert_normal_to_face(--norm_ind, face_index);
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
        triangulate_faces();
        calc_face_normals();
    }
} pol;

std::vector<polyhedron> pol_stack;

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

void draw_pols() {
    for (auto p : pol_stack)
        p.draw();
    pol.draw();
}

void save_pol_to_stack() {
    pol_stack.push_back(pol);
    pol = polyhedron();
}

void draw_UI() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 100));
    ImGui::Begin("Instruments", NULL,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse);

    static char filename[128] = "";
    ImGui::SetCursorPos(ImVec2(5, 27));
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("", filename, IM_ARRAYSIZE(filename));
    ImGui::SetCursorPos(ImVec2(5, 50));
    if (ImGui::Button("Load model", ImVec2(100, 20))) {
        pol.load_from_obj(filename);
    }
    ImGui::SetCursorPos(ImVec2(5, 73));
    if (ImGui::Button("Stack Pol", ImVec2(100, 20))) {
        save_pol_to_stack();
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

    static int light_pos_vals[3] = { 0, 0, 1 };
    ImGui::SetCursorPos(ImVec2(1120, 27));
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("light_x", &light_pos_vals[0])) {
        light_pos.x = light_pos_vals[0];
    }
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(1120, 52));
    if (ImGui::InputInt("light_y", &light_pos_vals[1])) {
        light_pos.y = light_pos_vals[1];
    }
    ImGui::SetCursorPos(ImVec2(1120, 77));
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("light_z", &light_pos_vals[2])) {
        light_pos.z = light_pos_vals[2];
    }

    static int obj_color_vals[3] = { 255, 255, 255 };
    ImGui::SetCursorPos(ImVec2(1290, 27));
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("obj_r", &obj_color_vals[0])) {
        obj_color_vals[0] = std::max(0, std::min(255, obj_color_vals[0]));
        pol.color.Value.x = obj_color_vals[0] / 255.0f;
    }
    ImGui::SetNextItemWidth(100);
    ImGui::SetCursorPos(ImVec2(1290, 52));
    if (ImGui::InputInt("obj_g", &obj_color_vals[1])) {
        obj_color_vals[1] = std::max(0, std::min(255, obj_color_vals[1]));
        pol.color.Value.y = obj_color_vals[1] / 255.0f;
    }
    ImGui::SetCursorPos(ImVec2(1290, 77));
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("obj_b", &obj_color_vals[2])) {
        obj_color_vals[2] = std::max(0, std::min(255, obj_color_vals[2]));
        pol.color.Value.z = obj_color_vals[2] / 255.0f;
    }

    ImGui::SetCursorPos(ImVec2(1460, 27));
    ImGui::SetNextItemWidth(100);
    if (ImGui::Checkbox("z_buffer", &use_z_buffer)) {
        if (!use_z_buffer)
            use_gouraud_shading = false;
    }
    ImGui::SetCursorPos(ImVec2(1460, 52));
    if (ImGui::Checkbox("gouraud_shading", &use_gouraud_shading)) {
        if (use_gouraud_shading) {
            use_z_buffer = true;
            use_phong_shading = false;
        }
    }
    ImGui::SetCursorPos(ImVec2(1460, 77));
    if (ImGui::Checkbox("phong_shading", &use_phong_shading)) {
        if (use_phong_shading) {
            use_z_buffer = true;
            use_gouraud_shading = false;
        }
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

            cur_view_vec = base_view_vec;
            cur_view_vec.affine_transformation(perspective_matrix);
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

            cur_view_vec = base_view_vec;
            cur_view_vec.affine_transformation(perspective_matrix);
        }
        else {
            projection_matr = base_proj_matr;
            pol.apply_view_matr(view_matr);
            isPerspec = false;

            cur_view_vec = base_view_vec;
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

void process_input(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera_angle_x += camera_rotation_speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera_angle_x -= camera_rotation_speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera_angle_y -= camera_rotation_speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera_angle_y += camera_rotation_speed;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera_z += camera_move_speed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera_z -= camera_move_speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera_x -= camera_move_speed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera_x += camera_move_speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera_y -= camera_move_speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera_y += camera_move_speed;

    if (camera_angle_x > M_PI / 2)
    {
        camera_angle_x = M_PI / 2;
    }

    if (camera_angle_x < -M_PI / 2)
    {
        camera_angle_x = -M_PI / 2;
    }

    if (camera_z < 0)
    {
        camera_z = 0;
    }
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTHSZ, HEIGHTSZ, "3D", NULL, NULL);
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
        process_input(window);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (use_z_buffer) {
            reset_z_buffer_related_variables();
            draw_pols();
            DrawScreen();
        }
        else {
            draw_pols();
        }

        view_matr = create_camera_view();
        auto res = matr_mult(projection_matr, view_matr);
        for (auto& p : pol_stack)
        {
            p.apply_view_matr(res);
        }
        pol.apply_view_matr(res);

        draw_UI();

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