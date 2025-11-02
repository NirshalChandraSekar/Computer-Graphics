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
    Vec3 operator/(double s)    const { return {x/s, y/s, z/s}; }
    Vec3 operator*(const Vec3& b) const { return {x*b.x, y*b.y, z*b.z}; }
    Vec3& operator+=(const Vec3& b){ x+=b.x; y+=b.y; z+=b.z; return *this; }
    double dot(const Vec3& b) const { return x*b.x + y*b.y + z*b.z; }
    Vec3 cross(const Vec3& b) const {
        return {
            y*b.z - z*b.y,
            z*b.x - x*b.z,
            x*b.y - y*b.x
        };
    }
    double norm() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const { double n = norm(); if (n<=1e-16) return *this; return {x/n,y/n,z/n}; }
};
static inline Vec3 reflect(const Vec3& I, const Vec3& N) { return I - N * (2.0 * I.dot(N)); }
struct Ray { Vec3 o, d; };

// Snell's law refraction. I is incoming (toward surface), N is outward normal (unit).
static inline bool refract(const Vec3& I, Vec3 N, double ior_out, double ior_in, Vec3& T){
    double cosi = std::max(-1.0, std::min(1.0, I.dot(N)));
    double etai = ior_out, etat = ior_in;
    Vec3 n = N;
    if (cosi > 0.0) { std::swap(etai, etat); n = N * -1.0; }
    double eta = etai / etat;
    double k = 1.0 - eta*eta * (1.0 - cosi*cosi);
    if (k < 0.0) return false; // total internal reflection
    T = I*eta + n*(eta*cosi - std::sqrt(k));
    return true;
}

// ---------------------- Scene Data ----------------------
struct Material {
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
    Vec3 color; 
    Vec3 dir;   
};
struct PointLight {
    Vec3 color;
    Vec3 pos;
};
struct SpotLight {
    Vec3 color;
    Vec3 pos;
    Vec3 dir;      
    double innerRad = 0.0; // radians
    double outerRad = 0.0; // radians
};

// --- Triangle primitives ---
struct TriFlat {
    int v0=-1, v1=-1, v2=-1; 
    Material mat;
};
struct TriSmooth {
    int v0=-1, v1=-1, v2=-1; 
    int n0=-1, n1=-1, n2=-1; 
    Material mat;
};

struct Scene {
    // Camera defaults per PDF
    Vec3 cam_pos{0,0,0};
    Vec3 cam_fwd{0,0,-1};  
    Vec3 cam_up{0,1,0};
    double cam_fov_ha_deg = 45.0; 
    int width = 640, height = 480;

    Vec3 background{0,0,0};
    Vec3 ambient_light{0,0,0};

    // Geometry
    std::vector<Vec3> vertices; // pool
    std::vector<Vec3> vnormals; // pool
    std::vector<Sphere> spheres;
    std::vector<TriFlat> triangles_flat;
    std::vector<TriSmooth> triangles_smooth;

    // Lights
    std::vector<DirectionalLight> dir_lights;
    std::vector<PointLight> point_lights;
    std::vector<SpotLight> spot_lights;

    std::string output_name = "raytraced.bmp";
    int max_depth = 10;
};

static inline unsigned char toU8(double x) {
    if (x < 0.0) x = 0.0; else if (x > 1.0) x = 1.0;
    return static_cast<unsigned char>(std::round(x * 255.0));
}

// ---------------------- Intersection / Hit ----------------------
struct Hit {
    double t = DBL_MAX;
    Vec3 P, N;
    const Material* mat = nullptr;
};

// Forward decls
static bool intersectScene(const Ray& ray, const Scene& sc, Hit& out);

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

        // -------- 3B: vertices / normals / triangles ----------
        } else if (cmd == "max_vertices:") {
            auto v = readAllDoubles(ss);
            if (!expect(1, v, cmd)) return false;
            scene.vertices.reserve((size_t)std::max(0, (int)std::round(v[0])));

        } else if (cmd == "max_normals:") {
            auto v = readAllDoubles(ss);
            if (!expect(1, v, cmd)) return false;
            scene.vnormals.reserve((size_t)std::max(0, (int)std::round(v[0])));

        } else if (cmd == "vertex:") {
            auto v = readAllDoubles(ss);
            if (!expect(3, v, cmd)) return false;
            scene.vertices.push_back(Vec3{v[0],v[1],v[2]});

        } else if (cmd == "normal:") {
            auto v = readAllDoubles(ss);
            if (!expect(3, v, cmd)) return false;
            scene.vnormals.push_back(Vec3{v[0],v[1],v[2]}.normalized());

        } else if (cmd == "triangle:") {
            auto v = readAllDoubles(ss);
            if (!expect(3, v, cmd)) return false;
            TriFlat t;
            t.v0 = (int)std::round(v[0]);
            t.v1 = (int)std::round(v[1]);
            t.v2 = (int)std::round(v[2]);
            t.mat = currentMat;
            if (t.v0<0 || t.v1<0 || t.v2<0 ||
                t.v0 >= (int)scene.vertices.size() ||
                t.v1 >= (int)scene.vertices.size() ||
                t.v2 >= (int)scene.vertices.size()) {
                std::cerr << "triangle index out of range at line " << lineNo << "\n";
                return false;
            }
            scene.triangles_flat.push_back(t);

        } else if (cmd == "normal_triangle:") {
            auto v = readAllDoubles(ss);
            if (!expect(6, v, cmd)) return false;
            TriSmooth t;
            t.v0 = (int)std::round(v[0]);
            t.v1 = (int)std::round(v[1]);
            t.v2 = (int)std::round(v[2]);
            t.n0 = (int)std::round(v[3]);
            t.n1 = (int)std::round(v[4]);
            t.n2 = (int)std::round(v[5]);
            t.mat = currentMat;
            if (t.v0<0 || t.v1<0 || t.v2<0 ||
                t.v0 >= (int)scene.vertices.size() ||
                t.v1 >= (int)scene.vertices.size() ||
                t.v2 >= (int)scene.vertices.size()) {
                std::cerr << "normal_triangle vertex index out of range at line " << lineNo << "\n";
                return false;
            }
            if (t.n0<0 || t.n1<0 || t.n2<0 ||
                t.n0 >= (int)scene.vnormals.size() ||
                t.n1 >= (int)scene.vnormals.size() ||
                t.n2 >= (int)scene.vnormals.size()) {
                std::cerr << "normal_triangle normal index out of range at line " << lineNo << "\n";
                return false;
            }
            scene.triangles_smooth.push_back(t);

        // -------- camera/film/output/depth ----------
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
            // ignore unknowns for forward-compat
        }
    }
    return true;
}

// ---------------------- Triangle Intersection (Möller–Trumbore) ----------------------
static inline bool intersectTriMT(const Ray& r, const Vec3& A, const Vec3& B, const Vec3& C,
                                  double& t, double& u, double& v)
{
    const double EPS = 1e-9;
    Vec3 ab = B - A, ac = C - A;
    Vec3 p  = r.d.cross(ac);
    double det = ab.dot(p);
    if (std::fabs(det) < EPS) return false;
    double inv = 1.0 / det;

    Vec3 s = r.o - A;
    u = s.dot(p) * inv; if (u < 0.0 || u > 1.0) return false;

    Vec3 q = s.cross(ab);
    v = r.d.dot(q) * inv; if (v < 0.0 || (u + v) > 1.0) return false;

    double tt = ac.dot(q) * inv;
    if (tt <= 1e-6) return false;

    t = tt;
    return true;
}

// ---------------------- Shading (with shadows) ----------------------
static inline void clamp01(Vec3& c){
    if (c.x<0) c.x=0; else if (c.x>1) c.x=1;
    if (c.y<0) c.y=0; else if (c.y>1) c.y=1;
    if (c.z<0) c.z=0; else if (c.z>1) c.z=1;
}

static inline bool shadowDirectional(const Vec3& P, const Vec3& Ldir, const Scene& sc){
    const double bias = 1e-4;
    Ray sray{ P + Ldir * bias, Ldir }; 
    Hit h;
    return intersectScene(sray, sc, h);
}

static inline bool shadowPointLike(const Vec3& P, const Vec3& Lpos, const Scene& sc){
    Vec3 toL = Lpos - P;
    double dist = toL.norm();
    if (dist <= 1e-9) return false;
    Vec3 dir = toL * (1.0/dist);
    const double bias = 1e-4;
    Ray sray{ P + dir*bias, dir };
    Hit h;
    if (!intersectScene(sray, sc, h)) return false;
    return h.t < dist - 1e-4; // occluder before the light
}

Vec3 shadePhongWithShadows(const Vec3& P, const Vec3& N, const Vec3& V,
                           const Material& m, const Scene& sc)
{
    Vec3 color = m.ambient * sc.ambient_light;

    // Directional lights (no attenuation)
    for (const auto& L : sc.dir_lights) {
        Vec3 Ldir = (L.dir * -1.0).normalized(); // from point to light
        if (shadowDirectional(P, Ldir, sc)) continue;

        double ndotl = std::max(0.0, N.dot(Ldir));
        Vec3 diff = m.diffuse * L.color * ndotl;

        Vec3 R = reflect(Ldir * -1.0, N);
        double rdotv = std::max(0.0, R.dot(V));
        Vec3 spec = m.specular * L.color * std::pow(rdotv, std::max(1.0, m.ns));

        color += diff + spec;
    }

    // Point lights (1/r^2 attenuation)
    for (const auto& L : sc.point_lights) {
        if (shadowPointLike(P, L.pos, sc)) continue;

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

    // Spot lights (cone factor + 1/r^2) + shadow
    for (const auto& L : sc.spot_lights) {
        if (shadowPointLike(P, L.pos, sc)) continue;

        Vec3 Lvec = L.pos - P;
        double dist2 = std::max(1e-12, Lvec.dot(Lvec));
        Vec3 Ldir = Lvec * (1.0 / std::sqrt(dist2));
        double atten = 1.0 / dist2;

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

// ---------------------- Scene Intersection ----------------------
static bool intersectScene(const Ray& ray, const Scene& sc, Hit& out) {
    bool hitAny = false;

    // Spheres
    for (const auto& s : sc.spheres) {
        double t; if (s.intersect(ray, t) && t < out.t) {
            out.t = t;
            out.P = ray.o + ray.d * t;
            out.N = (out.P - s.c).normalized();
            out.mat = &s.mat;
            hitAny = true;
        }
    }

    // Flat triangles
    for (const auto& tr : sc.triangles_flat) {
        const Vec3& A = sc.vertices[(size_t)tr.v0];
        const Vec3& B = sc.vertices[(size_t)tr.v1];
        const Vec3& C = sc.vertices[(size_t)tr.v2];
        double t,u,v;
        if (intersectTriMT(ray, A, B, C, t, u, v) && t < out.t) {
            out.t = t;
            out.P = ray.o + ray.d * t;
            Vec3 Ng = (B - A).cross(C - A).normalized();
            // Flip normal if it points in the same direction as the ray
            if (Ng.dot(ray.d) > 0.0) Ng = Ng * -1.0;
            out.N = Ng;
            out.mat = &tr.mat;
            hitAny = true;
        }
    }

    // Smooth triangles (barycentric-interpolated normal)
    for (const auto& tr : sc.triangles_smooth) {
        const Vec3& A = sc.vertices[(size_t)tr.v0];
        const Vec3& B = sc.vertices[(size_t)tr.v1];
        const Vec3& C = sc.vertices[(size_t)tr.v2];
        double t,u,v;
        if (intersectTriMT(ray, A, B, C, t, u, v) && t < out.t) {
            out.t = t;
            out.P = ray.o + ray.d * t;
            double w = 1.0 - u - v;
            const Vec3& n0 = sc.vnormals[(size_t)tr.n0];
            const Vec3& n1 = sc.vnormals[(size_t)tr.n1];
            const Vec3& n2 = sc.vnormals[(size_t)tr.n2];
            Vec3 Ninterp = (n0 * w + n1 * u + n2 * v).normalized();
            out.N = Ninterp;
            out.mat = &tr.mat;
            hitAny = true;
        }
    }

    return hitAny;
}

// ---------------------- Recursive Trace (reflections + refractions) ----------------------
static Vec3 trace(const Ray& ray, const Scene& sc, int depth) {
    if (depth <= 0) return {0,0,0};

    Hit h;
    if (!intersectScene(ray, sc, h)) return sc.background;

    Vec3 V = (ray.o - h.P).normalized();
    Vec3 local = shadePhongWithShadows(h.P, h.N, V, *h.mat, sc);

    // Reflection amount from specular color (per channel)
    Vec3 reflColor{0,0,0};
    double bias = 1e-4;
    if (h.mat->specular.x>0 || h.mat->specular.y>0 || h.mat->specular.z>0) {
        Vec3 Rdir = reflect(V * -1.0, h.N).normalized();
        Ray rray{ h.P + h.N * bias, Rdir };
        Vec3 rcol = trace(rray, sc, depth - 1);
        reflColor = Vec3{ rcol.x * h.mat->specular.x,
                          rcol.y * h.mat->specular.y,
                          rcol.z * h.mat->specular.z };
    }

    Vec3 refrColor{0,0,0};
    if ((h.mat->transmissive.x>0 || h.mat->transmissive.y>0 || h.mat->transmissive.z>0) &&
        h.mat->ior > 1.0) {
        Vec3 Tdir;
        // Air ior = 1.0
        if (refract(V * -1.0, h.N, 1.0, h.mat->ior, Tdir)) {
            Ray tray{ h.P - h.N * bias, Tdir.normalized() };
            Vec3 tcol = trace(tray, sc, depth - 1);
            refrColor = Vec3{ tcol.x * h.mat->transmissive.x,
                              tcol.y * h.mat->transmissive.y,
                              tcol.z * h.mat->transmissive.z };
        } else {
        }
    }

    Vec3 sum = local + reflColor + refrColor;
    clamp01(sum);
    return sum;
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
    Vec3 f  = (sc.cam_fwd * -1.0).normalized(); 
    Vec3 up =  sc.cam_up.normalized();
    Vec3 r  = up.cross(f).normalized();
    up      = r.cross(f).normalized();


    const int W = sc.width, H = sc.height;
    std::vector<unsigned char> img(W * H * 3, 0);

    double ha = sc.cam_fov_ha_deg * M_PI / 180.0;
    double half_h = std::tan(ha);
    double aspect = (double)W / (double)H;
    double half_w = aspect * half_h;

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            double u = ((x + 0.5) / (double)W) * 2.0 - 1.0; // [-1,1]
            double v = ((y + 0.5) / (double)H) * 2.0 - 1.0; // [-1,1]

            Vec3 dir = (f + r * (u * half_w) + up * (v * half_h)).normalized();
            Ray ray{sc.cam_pos, dir};
            Vec3 c = trace(ray, sc, sc.max_depth);

            size_t idx = (size_t(y) * W + size_t(x)) * 3;
            img[idx+0] = toU8(c.x);
            img[idx+1] = toU8(c.y);
            img[idx+2] = toU8(c.z);
        }
        if ((y % 32 == 0) || (y == H-1)) {
            double pct = 100.0 * (y+1) / H;
            std::cout << "\rRendering: " << (int)pct << "% " << std::flush;
        }
    }
    std::cout << "\n";

    // Write PNG
    std::string out = sc.output_name;
    if (out.size() >= 4) {
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
