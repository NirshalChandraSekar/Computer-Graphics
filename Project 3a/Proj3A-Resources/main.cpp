// raytracer.cpp
// Part-1 ray tracer per SceneFile.pdf: camera, spheres, background, material,
// ambient + directional + point + spot lights, max_depth parsing, multi-sphere.
// Usage: ./raytracer scene.txt

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// ---------------------- Math ----------------------
struct Vec3 {
    double x=0, y=0, z=0;
    Vec3() = default;
    Vec3(double X,double Y,double Z):x(X),y(Y),z(Z){}
    Vec3 operator+(const Vec3& b) const { return {x+b.x, y+b.y, z+b.z}; }
    Vec3 operator-(const Vec3& b) const { return {x-b.x, y-b.y, z-b.z}; }
    Vec3 operator*(double s)    const { return {x*s, y*s, z*s}; }
    Vec3 operator*(const Vec3& b) const { return {x*b.x, y*b.y, z*b.z}; } // Hadamard
    Vec3& operator+=(const Vec3& b){ x+=b.x; y+=b.y; z+=b.z; return *this; }
    double dot(const Vec3& b) const { return x*b.x + y*b.y + z*b.z; }
    double norm() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const { double n = norm(); if (n<=1e-16) return *this; return {x/n,y/n,z/n}; }
};
static inline Vec3 reflect(const Vec3& I, const Vec3& N) { return I - N * (2.0 * I.dot(N)); }
struct Ray { Vec3 o, d; };

// ---------------------- Scene Data ----------------------
struct Material {
    // (ar,ag,ab) ambient; (dr,dg,db) diffuse; (sr,sg,sb) specular; ns; (tr,tg,tb) transmissive; ior
    Vec3 ambient{0,0,0};
    Vec3 diffuse{1,1,1};
    Vec3 specular{0,0,0};
    double ns = 5.0;
    Vec3 transmissive{0,0,0};
    double ior = 1.0;
};

struct Sphere {
    Vec3 c; double r=1.0;
    Material mat;
    bool intersect(const Ray& ray, double& tHit) const {
        // Solve |o + t d - c|^2 = r^2 (d assumed normalized)
        Vec3 oc = ray.o - c;
        double b = oc.dot(ray.d);
        double cterm = oc.dot(oc) - r*r;
        double disc = b*b - cterm;
        if (disc < 0.0) return false;
        double s = std::sqrt(disc);
        double t0 = -b - s;
        double t1 = -b + s;
        double t = (t0 > 1e-6) ? t0 : ((t1 > 1e-6) ? t1 : -1.0);
        if (t < 0.0) return false;
        tHit = t;
        return true;
    }
};

struct DirectionalLight {
    Vec3 color; // intensity RGB
    Vec3 dir;   // light travels along +dir; for shading, Ldir = -dir (normalized)
};
struct PointLight {
    Vec3 color;
    Vec3 pos;
};
struct SpotLight {
    Vec3 color;
    Vec3 pos;
    Vec3 dir;       // spotlight forward direction (normalized)
    double innerRad = 0.0; // radians
    double outerRad = 0.0; // radians
};

struct Scene {
    // Camera defaults per PDF
    Vec3 cam_pos{0,0,0};
    Vec3 cam_fwd{0,0,-1};  // PDF default looks toward -Z
    Vec3 cam_up{0,1,0};
    double cam_fov_ha_deg = 45.0; // half-vertical FOV
    int width = 640, height = 480;

    Vec3 background{0,0,0};
    Vec3 ambient_light{0,0,0};

    std::vector<Sphere> spheres;
    std::vector<DirectionalLight> dir_lights;
    std::vector<PointLight> point_lights;
    std::vector<SpotLight> spot_lights;

    std::string output_name = "raytraced.bmp"; // PDF default name
    int max_depth = 5; // parsed for completeness (no recursion used in 3A)
};

static inline unsigned char toU8(double x) {
    if (x < 0.0) x = 0.0; else if (x > 1.0) x = 1.0;
    return static_cast<unsigned char>(std::round(x * 255.0));
}

// ---------------------- Parsing ----------------------
bool parseScene(const std::string& filename, Scene& scene) {
    std::ifstream in(filename);
    if (!in) { std::cerr << "Failed to open scene file: " << filename << "\n"; return false; }

    Material currentMat; // "current material" state
    std::string line; int lineNo = 0;

    auto readAllDoubles = [](std::istringstream& s){
        std::vector<double> v; double x; while (s >> x) v.push_back(x); return v;
    };
    auto expect = [&](int n, const std::vector<double>& v, const std::string& cmd){
        if ((int)v.size() != n) {
            std::cerr << "Parse error line " << lineNo << " ("<<cmd<<"): expected " << n << " numbers, got " << v.size() << "\n";
            return false;
        }
        return true;
    };

    while (std::getline(in, line)) {
        ++lineNo;
        size_t p = line.find_first_not_of(" \t\r\n");
        if (p == std::string::npos) continue;
        if (line[p] == '#') continue;

        std::istringstream ss(line);
        std::string cmd; ss >> cmd;
        if (cmd.empty()) continue;

        if (cmd == "material:") {
            // ar ag ab dr dg db sr sg sb ns tr tg tb ior
            auto v = readAllDoubles(ss);
            if (!expect(14, v, cmd)) return false;
            currentMat.ambient  = {v[0], v[1], v[2]};
            currentMat.diffuse  = {v[3], v[4], v[5]};
            currentMat.specular = {v[6], v[7], v[8]};
            currentMat.ns       =  v[9];
            currentMat.transmissive = {v[10], v[11], v[12]};
            currentMat.ior      =  v[13];

        } else if (cmd == "sphere:") {
            // x y z r
            auto v = readAllDoubles(ss);
            if (!expect(4, v, cmd)) return false;
            Sphere s; s.c = {v[0],v[1],v[2]}; s.r = v[3]; s.mat = currentMat;
            scene.spheres.push_back(s);

        } else if (cmd == "background:") {
            auto v = readAllDoubles(ss);
            if (!expect(3, v, cmd)) return false;
            scene.background = {v[0], v[1], v[2]};

        } else if (cmd == "ambient_light:") {
            auto v = readAllDoubles(ss);
            if (!expect(3, v, cmd)) return false;
            scene.ambient_light = {v[0], v[1], v[2]};

        } else if (cmd == "directional_light:") {
            // r g b  x y z  (dir where light travels)
            auto v = readAllDoubles(ss);
            if (!expect(6, v, cmd)) return false;
            DirectionalLight L; L.color = {v[0],v[1],v[2]}; L.dir = Vec3{v[3],v[4],v[5]}.normalized();
            scene.dir_lights.push_back(L);

        } else if (cmd == "point_light:") {
            // r g b  x y z
            auto v = readAllDoubles(ss);
            if (!expect(6, v, cmd)) return false;
            PointLight L; L.color = {v[0],v[1],v[2]}; L.pos = {v[3],v[4],v[5]};
            scene.point_lights.push_back(L);

        } else if (cmd == "spot_light:") {
            // r g b  px py pz  dx dy dz  angle1 angle2 (deg)
            auto v = readAllDoubles(ss);
            if (!expect(11, v, cmd)) return false;
            SpotLight L;
            L.color = {v[0],v[1],v[2]};
            L.pos   = {v[3],v[4],v[5]};
            L.dir   = Vec3{v[6],v[7],v[8]}.normalized();
            double a1 = v[9]  * M_PI/180.0;
            double a2 = v[10] * M_PI/180.0;
            if (a1 > a2) std::swap(a1, a2);
            L.innerRad = a1; L.outerRad = a2;
            scene.spot_lights.push_back(L);

        } else if (cmd == "camera_pos:") {
            auto v = readAllDoubles(ss); if (!expect(3, v, cmd)) return false; scene.cam_pos = {v[0],v[1],v[2]};

        } else if (cmd == "camera_fwd:") {
            auto v = readAllDoubles(ss); if (!expect(3, v, cmd)) return false; scene.cam_fwd = Vec3{v[0],v[1],v[2]}.normalized();

        } else if (cmd == "camera_up:") {
            auto v = readAllDoubles(ss); if (!expect(3, v, cmd)) return false; scene.cam_up = Vec3{v[0],v[1],v[2]}.normalized();

        } else if (cmd == "camera_fov_ha:") {
            auto v = readAllDoubles(ss); if (!expect(1, v, cmd)) return false; scene.cam_fov_ha_deg = v[0];

        } else if (cmd == "film_resolution:") {
            auto v = readAllDoubles(ss); if (!expect(2, v, cmd)) return false;
            scene.width = std::max(1, (int)std::round(v[0]));
            scene.height= std::max(1, (int)std::round(v[1]));

        } else if (cmd == "output_image:") {
            std::string rest; std::getline(ss, rest);
            size_t b = rest.find_first_not_of(" \t\r\n\"");
            size_t e = rest.find_last_not_of(" \t\r\n\"");
            if (b != std::string::npos && e != std::string::npos && e >= b)
                scene.output_name = rest.substr(b, e - b + 1);

        } else if (cmd == "max_depth:") {
            auto v = readAllDoubles(ss); if (!expect(1, v, cmd)) return false; scene.max_depth = (int)std::round(v[0]);

        } else {
            // ignore unknowns for forward-compat (e.g., triangles for 3B)
        }
    }
    return true;
}

// ---------------------- Shading ----------------------
static inline void clamp01(Vec3& c){
    if (c.x<0) c.x=0; else if (c.x>1) c.x=1;
    if (c.y<0) c.y=0; else if (c.y>1) c.y=1;
    if (c.z<0) c.z=0; else if (c.z>1) c.z=1;
}

Vec3 shadePhong(const Vec3& P, const Vec3& N, const Vec3& V,
                const Material& m, const Scene& sc)
{
    // Ambient
    Vec3 color = m.ambient * sc.ambient_light;

    // Directional lights (no attenuation)
    for (const auto& L : sc.dir_lights) {
        Vec3 Ldir = (L.dir * -1.0).normalized(); // light-to-point direction
        double ndotl = std::max(0.0, N.dot(Ldir));
        Vec3 diff = m.diffuse * L.color * ndotl;

        Vec3 R = reflect(Ldir * -1.0, N);
        double rdotv = std::max(0.0, R.dot(V));
        Vec3 spec = m.specular * L.color * std::pow(rdotv, std::max(1.0, m.ns));

        color += diff + spec;
    }

    // Point lights (1/r^2 attenuation)
    for (const auto& L : sc.point_lights) {
        Vec3 Lvec = L.pos - P;
        double dist2 = std::max(1e-12, Lvec.dot(Lvec));
        Vec3 Ldir = Lvec * (1.0 / std::sqrt(dist2));
        double atten = 1.0 / dist2;

        double ndotl = std::max(0.0, N.dot(Ldir));
        Vec3 diff = m.diffuse * L.color * (ndotl * atten);

        Vec3 R = reflect(Ldir * -1.0, N);
        double rdotv = std::max(0.0, R.dot(V));
        Vec3 spec = m.specular * L.color * (std::pow(rdotv, std::max(1.0, m.ns)) * atten);

        color += diff + spec;
    }

    // Spot lights (cone factor + 1/r^2)
    for (const auto& L : sc.spot_lights) {
        Vec3 Lvec = L.pos - P;
        double dist2 = std::max(1e-12, Lvec.dot(Lvec));
        Vec3 Ldir = Lvec * (1.0 / std::sqrt(dist2));
        double atten = 1.0 / dist2;

        // Cone
        double cosAng = L.dir.normalized().dot(Ldir);
        double theta  = std::acos(std::max(-1.0, std::min(1.0, cosAng)));
        double spot = 0.0;
        if (theta <= L.innerRad) spot = 1.0;
        else if (theta >= L.outerRad) spot = 0.0;
        else {
            double t = (theta - L.innerRad) / (L.outerRad - L.innerRad);
            spot = 1.0 - t; 
        }
        if (spot <= 0.0) continue;

        double ndotl = std::max(0.0, N.dot(Ldir));
        Vec3 diff = m.diffuse * L.color * (ndotl * atten * spot);

        Vec3 R = reflect(Ldir * -1.0, N);
        double rdotv = std::max(0.0, R.dot(V));
        Vec3 spec = m.specular * L.color * (std::pow(rdotv, std::max(1.0, m.ns)) * atten * spot);

        color += diff + spec;
    }

    clamp01(color);
    return color;
}

// ---------------------- Ray Casting ----------------------
Vec3 tracePrimary(const Ray& ray, const Scene& sc) {
    double closestT = DBL_MAX;
    const Sphere* hit = nullptr;

    for (const auto& s : sc.spheres) {
        double t; if (s.intersect(ray, t) && t < closestT) { closestT = t; hit = &s; }
    }
    if (!hit) return sc.background;

    Vec3 P = ray.o + ray.d * closestT;
    Vec3 N = (P - hit->c).normalized();
    Vec3 V = (ray.o - P).normalized(); // to camera

    return shadePhong(P, N, V, hit->mat, sc);
}

// ---------------------- Main ----------------------
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << (argc>0 ? argv[0] : "raytracer") << " scene.txt\n";
        return 1;
    }
    Scene sc;
    if (!parseScene(argv[1], sc)) { std::cerr << "Failed to parse scene.\n"; return 1; }

    // Camera basis (right-handed basis from given fwd & up)
    Vec3 f = sc.cam_fwd.normalized();
    Vec3 up = sc.cam_up.normalized();
    Vec3 r = Vec3{
                    f.y*up.z - f.z*up.y,
                    f.z*up.x - f.x*up.z,
                    f.x*up.y - f.y*up.x
                }.normalized();
    up = Vec3{
                r.y*f.z - r.z*f.y,
                r.z*f.x - r.x*f.z,
                r.x*f.y - r.y*f.x
            }.normalized();

    const int W = sc.width, H = sc.height;
    std::vector<unsigned char> img(W * H * 3, 0);

    double ha = sc.cam_fov_ha_deg * M_PI / 180.0;
    double half_h = std::tan(ha);
    double aspect = (double)W / (double)H;
    double half_w = aspect * half_h;

    // Render (1 sample per pixel)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            double u = ((x + 0.5) / (double)W) * 2.0 - 1.0; // [-1,1]
            double v = ((y + 0.5) / (double)H) * 2.0 - 1.0; // [-1,1]
            v = -v; // image-space up

            Vec3 dir = (f + r * (u * half_w) + up * (v * half_h)).normalized();
            Ray ray{sc.cam_pos, dir};
            Vec3 c = tracePrimary(ray, sc);

            size_t idx = (size_t(y) * W + size_t(x)) * 3;
            img[idx+0] = toU8(c.x);
            img[idx+1] = toU8(c.y);
            img[idx+2] = toU8(c.z);
        }
        if (y % 32 == 0) {
            double pct = 100.0 * (y+1) / H;
            std::cout << "\rRendering: " << (int)pct << "% " << std::flush;
        }
    }
    std::cout << "\n";

    // Write PNG (honors output_image extension if you prefer .png)
    std::string out = sc.output_name;
    if (out.size() >= 4) {
        // If user gave .bmp, we'll still write PNG with same name (or you can detect & write BMP).
        // Simple path: always write PNG; change name if extension not .png
        if (out.substr(out.size()-4) == ".bmp") out = out.substr(0, out.size()-4) + ".png";
    } else {
        out = "raytraced.png";
    }
    if (!stbi_write_png(out.c_str(), W, H, 3, img.data(), W*3)) {
        std::cerr << "Failed to write image: " << out << "\n"; return 1;
    }
    std::cout << "Wrote " << out << " (" << W << "x" << H << ")\n";
    return 0;
}
