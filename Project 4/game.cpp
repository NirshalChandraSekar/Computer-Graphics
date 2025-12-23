// In this file I basically set up OpenGL, load a text map, load a few meshes,
// and then do a simple 3D first-person maze with keys and doors.

#include "glad/glad.h"
#if defined(__APPLE__) || defined(__linux__)
 #include <SDL3/SDL.h>
 #include <SDL3/SDL_opengl.h>
#else
 #include <SDL.h>
 #include <SDL_opengl.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>
#include <cctype>
#include <algorithm>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



const char* vertexShaderSrc = R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vTexCoord;

void main() {
    vec4 worldPos = uModel * vec4(inPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * inNormal;
    vTexCoord = inTexCoord;
    gl_Position = uProj * uView * worldPos;
}
)";


const char* fragmentShaderSrc = R"(#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vTexCoord;

out vec4 fragColor;

uniform vec3 uColor;
uniform vec3 uLightDir;
uniform vec3 uViewPos;

uniform sampler2D uWallTex;   // used for any texture bound to unit 0
uniform int uUseTexture;      // 0 = solid color, 1 = use texture

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    float diff = max(dot(N, L), 0.0);

    float ambient = 0.3;
    float diffuse = 0.7 * diff;

    vec3 baseColor = uColor;
    if (uUseTexture == 1) {
        baseColor = texture(uWallTex, vTexCoord).rgb;
    }

    vec3 color = baseColor * (ambient + diffuse);
    fragColor = vec4(color, 1.0);
}
)";


// I keep the map as text lines

struct Map {
    int width  = 0;
    int height = 0;
    std::vector<std::string> grid;
    glm::vec2 start = glm::vec2(0.5f, 0.5f);
    glm::vec2 goal  = glm::vec2(0.5f, 0.5f);
};

// Here I load the map from a text file: first width/height, then rows.

bool loadMap(const std::string& path, Map& map) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Failed to open map file: " << path << "\n";
        return false;
    }

    int w, h;
    in >> w >> h;
    if (!in) {
        std::cerr << "Failed to read width/height from map\n";
        return false;
    }

    std::string line;
    std::getline(in, line);

    map.width = w;
    map.height = h;
    map.grid.clear();
    map.grid.reserve(h);

    for (int row = 0; row < h; ++row) {
        std::getline(in, line);
        if (!in) {
            std::cerr << "Not enough lines in map file\n";
            return false;
        }

        if ((int)line.size() < w) {
            std::string compact;
            for (char c : line) {
                if (!std::isspace((unsigned char)c)) compact.push_back(c);
            }
            line = compact;
        }

        if ((int)line.size() < w) {
            std::cerr << "Map line " << row << " too short\n";
            return false;
        }

        line = line.substr(0, w);
        map.grid.push_back(line);

        for (int col = 0; col < w; ++col) {
            char c = line[col];
            if (c == 'S') {
                map.start = glm::vec2(col + 0.5f, row + 0.5f);
            } else if (c == 'G') {
                map.goal = glm::vec2(col + 0.5f, row + 0.5f);
            }
        }
    }

    return true;
}

// I treat outside map or walls/doors as solid.

bool isSolid(const Map& map, int x, int z) {
    if (x < 0 || x >= map.width || z < 0 || z >= map.height) return true;
    char c = map.grid[z][x];
    if (c == 'W') return true;
    if (c >= 'A' && c <= 'E') return true; // treat doors as walls (until unlocked)
    return false;
}


// I build a cube as a list of triangles
// Each vertex has position, normal, and UV.

void createCube(std::vector<float>& vertices) {
    // pos(x,y,z), normal(nx,ny,nz), uv(u,v)
    const float data[] = {
        // +X
        0.5f,-0.5f,-0.5f,  1,0,0,  0,0,
        0.5f, 0.5f,-0.5f,  1,0,0,  0,1,
        0.5f, 0.5f, 0.5f,  1,0,0,  1,1,
        0.5f,-0.5f,-0.5f,  1,0,0,  0,0,
        0.5f, 0.5f, 0.5f,  1,0,0,  1,1,
        0.5f,-0.5f, 0.5f,  1,0,0,  1,0,

        // -X
       -0.5f,-0.5f, 0.5f, -1,0,0,  0,0,
       -0.5f, 0.5f, 0.5f, -1,0,0,  0,1,
       -0.5f, 0.5f,-0.5f, -1,0,0,  1,1,
       -0.5f,-0.5f, 0.5f, -1,0,0,  0,0,
       -0.5f, 0.5f,-0.5f, -1,0,0,  1,1,
       -0.5f,-0.5f,-0.5f, -1,0,0,  1,0,

        // +Y
       -0.5f, 0.5f,-0.5f, 0,1,0,  0,0,
       -0.5f, 0.5f, 0.5f, 0,1,0,  0,1,
        0.5f, 0.5f, 0.5f, 0,1,0,  1,1,
       -0.5f, 0.5f,-0.5f, 0,1,0,  0,0,
        0.5f, 0.5f, 0.5f, 0,1,0,  1,1,
        0.5f, 0.5f,-0.5f, 0,1,0,  1,0,

        // -Y
       -0.5f,-0.5f, 0.5f, 0,-1,0, 0,0,
       -0.5f,-0.5f,-0.5f,0,-1,0, 0,1,
        0.5f,-0.5f,-0.5f,0,-1,0, 1,1,
       -0.5f,-0.5f, 0.5f,0,-1,0, 0,0,
        0.5f,-0.5f,-0.5f,0,-1,0, 1,1,
        0.5f,-0.5f, 0.5f,0,-1,0, 1,0,

        // +Z
       -0.5f,-0.5f, 0.5f, 0,0,1,  0,0,
        0.5f,-0.5f, 0.5f, 0,0,1,  1,0,
        0.5f, 0.5f, 0.5f, 0,0,1,  1,1,
       -0.5f,-0.5f, 0.5f, 0,0,1,  0,0,
        0.5f, 0.5f, 0.5f, 0,0,1,  1,1,
       -0.5f, 0.5f, 0.5f, 0,0,1,  0,1,

        // -Z
        0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,
       -0.5f,-0.5f,-0.5f, 0,0,-1, 1,0,
       -0.5f, 0.5f,-0.5f, 0,0,-1, 1,1,
        0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,
       -0.5f, 0.5f,-0.5f, 0,0,-1, 1,1,
        0.5f, 0.5f,-0.5f, 0,0,-1, 0,1
    };
    vertices.assign(data, data + sizeof(data) / sizeof(float));
}

// I also make a simple square plane for the floor (two triangles).

void createFloor(std::vector<float>& vertices) {
    const float data[] = {
        -1.0f, 0.0f,-1.0f,  0,1,0,  0,0,
         1.0f, 0.0f,-1.0f,  0,1,0,  1,0,
         1.0f, 0.0f, 1.0f,  0,1,0,  1,1,
        -1.0f, 0.0f,-1.0f,  0,1,0,  0,0,
         1.0f, 0.0f, 1.0f,  0,1,0,  1,1,
        -1.0f, 0.0f, 1.0f,  0,1,0,  0,1
    };
    vertices.assign(data, data + sizeof(data) / sizeof(float));
}


// The below code loads meshes from either simple txt format or OBJ format.
// I had to use the help of ChatGPT to get the OBJ loader working correctly
// Because my initial implementation only handled simple OBJ files with triangles.

bool loadTxtMesh(const std::string& path, std::vector<float>& outVerts, int& floatsPerVertex) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Failed to open txt mesh: " << path << "\n";
        return false;
    }
    int count;
    in >> count;
    if (!in || count <= 0) {
        std::cerr << "Mesh txt header invalid in " << path << "\n";
        return false;
    }
    outVerts.resize(count);
    for (int i = 0; i < count; ++i) {
        in >> outVerts[i];
        if (!in) {
            std::cerr << "Mesh txt file ended early in " << path << "\n";
            return false;
        }
    }
    floatsPerVertex = 8;
    return true;
}


bool loadObjMesh(const std::string& path, std::vector<float>& outVerts, int& floatsPerVertex) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Failed to open OBJ: " << path << "\n";
        return false;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> normals;

    std::string line;

    auto toIntSafe = [](const std::string& s) -> int {
        if (s.empty()) return 0;
        return std::stoi(s);
    };

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            positions.emplace_back(x, y, z);
        } else if (tag == "vt") {
            float u, v;
            iss >> u >> v;
            texcoords.emplace_back(u, v);
        } else if (tag == "vn") {
            float x, y, z;
            iss >> x >> y >> z;
            normals.emplace_back(x, y, z);
        } else if (tag == "f") {
            std::vector<int> vIdxs;
            std::vector<int> tIdxs;
            std::vector<int> nIdxs;

            std::string vertSpec;
            while (iss >> vertSpec) {
                if (vertSpec.empty()) continue;

                int vIdx = 0, tIdx = 0, nIdx = 0;

                size_t s1 = vertSpec.find('/');
                size_t s2 = std::string::npos;
                if (s1 != std::string::npos) {
                    s2 = vertSpec.find('/', s1 + 1);
                }

                if (s1 == std::string::npos) {
                    vIdx = toIntSafe(vertSpec);
                } else {
                    std::string vStr = vertSpec.substr(0, s1);
                    std::string vtStr, vnStr;
                    if (s2 == std::string::npos) {
                        vtStr = vertSpec.substr(s1 + 1);
                    } else {
                        vtStr = vertSpec.substr(s1 + 1, s2 - s1 - 1);
                        vnStr = vertSpec.substr(s2 + 1);
                    }
                    vIdx = toIntSafe(vStr);
                    tIdx = toIntSafe(vtStr);
                    nIdx = toIntSafe(vnStr);
                }

                vIdxs.push_back(vIdx);
                tIdxs.push_back(tIdx);
                nIdxs.push_back(nIdx);
            }

            if (vIdxs.size() < 3) continue;

            auto emitVertex = [&](int vIdx, int tIdx, int nIdx) {
                if (vIdx <= 0 || vIdx > (int)positions.size()) {
                    std::cerr << "OBJ has invalid vertex index in " << path << "\n";
                    return;
                }

                glm::vec3 p = positions[vIdx - 1];

                glm::vec2 uv(0.0f, 0.0f);
                if (tIdx > 0 && tIdx <= (int)texcoords.size()) {
                    uv = texcoords[tIdx - 1];
                }

                glm::vec3 n(0.0f, 1.0f, 0.0f);
                if (nIdx > 0 && nIdx <= (int)normals.size()) {
                    n = normals[nIdx - 1];
                }

                outVerts.push_back(p.x);
                outVerts.push_back(p.y);
                outVerts.push_back(p.z);
                outVerts.push_back(uv.x);
                outVerts.push_back(uv.y);
                outVerts.push_back(n.x);
                outVerts.push_back(n.y);
                outVerts.push_back(n.z);
            };

            for (size_t i = 1; i + 1 < vIdxs.size(); ++i) {
                emitVertex(vIdxs[0],        tIdxs[0],        nIdxs[0]);
                emitVertex(vIdxs[i],        tIdxs[i],        nIdxs[i]);
                emitVertex(vIdxs[i + 1],    tIdxs[i + 1],    nIdxs[i + 1]);
            }
        }
    }

    if (outVerts.empty()) {
        std::cerr << "OBJ produced no vertices: " << path << "\n";
        return false;
    }

    floatsPerVertex = 8;
    return true;
}


bool loadMesh(const std::string& path, std::vector<float>& outVerts, int& floatsPerVertex) {
    outVerts.clear();
    floatsPerVertex = 0;

    std::string ext;
    size_t dot = path.find_last_of('.');
    if (dot != std::string::npos) {
        ext = path.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    if (ext == "obj") {
        return loadObjMesh(path, outVerts, floatsPerVertex);
    } else {
        // default: txt-style
        return loadTxtMesh(path, outVerts, floatsPerVertex);
    }
}


GLuint buildShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        std::cerr << "Shader compile error: " << log << "\n";
    }
    return s;
}

GLuint buildProgram() {
    GLuint vs = buildShader(GL_VERTEX_SHADER,   vertexShaderSrc);
    GLuint fs = buildShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "Program link error: " << log << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// camera setup that gives the first person view
struct Camera {
    glm::vec3 pos;
    float yaw; // radians
};




int main(int argc, char** argv) {

    std::string mapPath = "map.txt";
    if (argc > 1) mapPath = argv[1];

    Map map;
    if (!loadMap(mapPath, map)) {
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow(
        "Project 4 - SDL3 Maze Viewer",
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glctx = SDL_GL_CreateContext(window);
    if (!glctx) {
        std::cerr << "Failed to create GL context: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        SDL_GL_DestroyContext(glctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);

    GLuint program = buildProgram();
    glUseProgram(program);

    GLint locUseTexture = glGetUniformLocation(program, "uUseTexture");
    GLint locWallTex    = glGetUniformLocation(program, "uWallTex");
    GLint locModel      = glGetUniformLocation(program, "uModel");
    GLint locView       = glGetUniformLocation(program, "uView");
    GLint locProj       = glGetUniformLocation(program, "uProj");
    GLint locColor      = glGetUniformLocation(program, "uColor");
    GLint locLightDir   = glGetUniformLocation(program, "uLightDir");
    GLint locViewPos    = glGetUniformLocation(program, "uViewPos");

    glUniform1i(locWallTex, 0); 


    int texW, texH, texC;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("MultiObjectTextures/wall_green.jpg",
                                    &texW, &texH, &texC, 4);
    if (!data) {
        std::cerr << "Failed to load wall texture\n";
    }

    GLuint wallTex;
    glGenTextures(1, &wallTex);
    glBindTexture(GL_TEXTURE_2D, wallTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);


    int floorW, floorH, floorC;
    unsigned char* floorData = stbi_load("MultiObjectTextures/floor_green.jpg",
                                         &floorW, &floorH, &floorC, 4);
    if (!floorData) {
        std::cerr << "Failed to load floor texture\n";
    }

    GLuint floorTex;
    glGenTextures(1, &floorTex);
    glBindTexture(GL_TEXTURE_2D, floorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, floorW, floorH, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, floorData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(floorData);

    // ====== NEW: load night-sky texture for background (skybox) ======
    int skyW, skyH, skyC;
    unsigned char* skyData = stbi_load("MultiObjectTextures/night_sky.jpg",
                                       &skyW, &skyH, &skyC, 4);
    if (!skyData) {
        std::cerr << "Failed to load sky texture\n";
    }

    GLuint skyTex;
    glGenTextures(1, &skyTex);
    glBindTexture(GL_TEXTURE_2D, skyTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, skyW, skyH, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, skyData);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(skyData);
    // ===============================================================

    
    std::vector<float> doorVerts;
    int doorFloatsPerVertex = 0;

    if (!loadMesh("models/knot.txt", doorVerts, doorFloatsPerVertex)) {
        std::cerr << "Failed to load door model\n";
    }

    GLuint doorVAO, doorVBO;
    glGenVertexArrays(1, &doorVAO);
    glGenBuffers(1, &doorVBO);

    glBindVertexArray(doorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, doorVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 doorVerts.size() * sizeof(float),
                 doorVerts.data(),
                 GL_STATIC_DRAW);


    GLsizei doorStride = doorFloatsPerVertex * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, doorStride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, doorStride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(1);


    glBindVertexArray(0);

    int doorVertexCount = (doorFloatsPerVertex > 0)
        ? (int)(doorVerts.size() / doorFloatsPerVertex)
        : 0;



    std::vector<float> goalVerts;
    int goalFloatsPerVertex = 0;

    if (!loadMesh("models/teapot.txt", goalVerts, goalFloatsPerVertex)) {
        std::cerr << "Failed to load goal model\n";
    }

    GLuint goalVAO, goalVBO;
    glGenVertexArrays(1, &goalVAO);
    glGenBuffers(1, &goalVBO);
    glBindVertexArray(goalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, goalVBO);
    glBufferData(GL_ARRAY_BUFFER, goalVerts.size() * sizeof(float), goalVerts.data(), GL_STATIC_DRAW);

    GLsizei goalStride = goalFloatsPerVertex * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, goalStride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, goalStride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    int goalVertexCount = (goalFloatsPerVertex > 0)
        ? (int)(goalVerts.size() / goalFloatsPerVertex)
        : 0;


    std::vector<float> keyVerts;
    int keyFloatsPerVertex = 0;

    if (!loadMesh("models/s.obj", keyVerts, keyFloatsPerVertex)) {
        std::cerr << "Failed to load key model\n";
    }

    GLuint keyVAO, keyVBO;
    glGenVertexArrays(1, &keyVAO);
    glGenBuffers(1, &keyVBO);

    glBindVertexArray(keyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, keyVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 keyVerts.size() * sizeof(float),
                 keyVerts.data(),
                 GL_STATIC_DRAW);


    GLsizei keyStride = keyFloatsPerVertex * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, keyStride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, keyStride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    int keyVertexCount = (keyFloatsPerVertex > 0)
        ? (int)(keyVerts.size() / keyFloatsPerVertex)
        : 0;

    
    std::vector<float> cubeVerts;
    createCube(cubeVerts);
    GLuint cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, cubeVerts.size() * sizeof(float), cubeVerts.data(), GL_STATIC_DRAW);

    GLsizei stride = 8 * sizeof(float); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

  

    std::vector<float> floorVerts;
    createFloor(floorVerts);
    GLuint floorVAO, floorVBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, floorVerts.size() * sizeof(float), floorVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

   

    Camera cam;
    cam.pos = glm::vec3(map.start.x, 0.5f, map.start.y);
    cam.yaw = 0.0f;

    
    bool hasKey[5] = {false, false, false, false, false};
    int currentKeyIndex = -1; 

    Uint64 prevTicks = SDL_GetTicksNS();
    bool running = true;
    float goalAngle = 0.0f;  

    while (running) {
        Uint64 nowTicks = SDL_GetTicksNS();
        double dt = (nowTicks - prevTicks) / 1e9;
        prevTicks = nowTicks;

        goalAngle += (float)(dt * 1.0f); 

        
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }


        const bool* state = SDL_GetKeyboardState(nullptr);

        float moveSpeed = 3.0f;
        float turnSpeed = 1.5f;

        if (state[SDL_SCANCODE_LEFT]) {
            cam.yaw -= turnSpeed * (float)dt;
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            cam.yaw += turnSpeed * (float)dt;
        }

        glm::vec3 forward = glm::vec3(std::cos(cam.yaw), 0.0f, std::sin(cam.yaw));
        glm::vec3 right   = glm::vec3(-forward.z, 0.0f, forward.x);

        glm::vec3 newPos = cam.pos;
        if (state[SDL_SCANCODE_W] || state[SDL_SCANCODE_UP]) {
            newPos += forward * (moveSpeed * (float)dt);
        }
        if (state[SDL_SCANCODE_S] || state[SDL_SCANCODE_DOWN]) {
            newPos -= forward * (moveSpeed * (float)dt);
        }
        if (state[SDL_SCANCODE_A]) {
            newPos -= right * (moveSpeed * (float)dt);
        }
        if (state[SDL_SCANCODE_D]) {
            newPos += right * (moveSpeed * (float)dt);
        }

        const float COLLISION_MARGIN = 0.2f;
        float dx = newPos.x - cam.pos.x;
        float dz = newPos.z - cam.pos.z;
        float offsetX = (dx > 0.0f) ? COLLISION_MARGIN : (dx < 0.0f ? -COLLISION_MARGIN : 0.0f);
        float offsetZ = (dz > 0.0f) ? COLLISION_MARGIN : (dz < 0.0f ? -COLLISION_MARGIN : 0.0f);

        int mx = (int)std::floor(newPos.x + offsetX);
        int mz = (int)std::floor(newPos.z + offsetZ);

        if (!isSolid(map, mx, mz)) {
            cam.pos = newPos;
        }

        
        {
            const float pickupRadius = 0.4f;
            const float pickupRadiusSq = pickupRadius * pickupRadius;
            glm::vec2 playerPos(cam.pos.x, cam.pos.z);

            for (int z = 0; z < map.height; ++z) {
                for (int x = 0; x < map.width; ++x) {
                    char c = map.grid[z][x];
                    if (c >= 'a' && c <= 'e') {
                        glm::vec2 keyPos(x + 0.5f, z + 0.5f);
                        float kdx = playerPos.x - keyPos.x;
                        float kdz = playerPos.y - keyPos.y;
                        float distSq = kdx * kdx + kdz * kdz;
                        if (distSq < pickupRadiusSq) {
                            int keyIndex = c - 'a';
                            hasKey[keyIndex] = true;
                            currentKeyIndex = keyIndex; 
                            map.grid[z][x] = '.';       
                        }
                    }
                }
            }
        }

        int winW, winH;
        SDL_GetWindowSizeInPixels(window, &winW, &winH);
        float aspect = (winH > 0) ? (float)winW / (float)winH : 16.0f / 9.0f;
        glViewport(0, 0, winW, winH);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);

        glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
        glm::vec3 eye = cam.pos;
        glm::vec3 center = cam.pos + forward;
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::mat4 view = glm::lookAt(eye, center, up);

        // ----- Draw skybox (night sky) FIRST -----
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));

        glDepthMask(GL_FALSE); // don't write depth for the skybox

        glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(viewNoTrans));

        glBindVertexArray(cubeVAO);

        glm::mat4 skyModel(1.0f);
        skyModel = glm::translate(skyModel, cam.pos);       // center on camera
        skyModel = glm::scale(skyModel, glm::vec3(50.0f));  // big cube around player
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(skyModel));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyTex);
        glUniform1i(locUseTexture, 1);
        glUniform3f(locColor, 1.0f, 1.0f, 1.0f);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        glDepthMask(GL_TRUE); // re-enable depth writing

        // ----- Now normal view + lighting for the maze -----
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3f(locLightDir, -0.3f, -1.0f, -0.2f);
        glUniform3f(locViewPos, eye.x, eye.y, eye.z);

        glm::vec3 doorColors[5] = {
            glm::vec3(0.8f, 0.2f, 0.2f),
            glm::vec3(0.2f, 0.8f, 0.2f),
            glm::vec3(0.2f, 0.2f, 0.8f),
            glm::vec3(0.8f, 0.8f, 0.2f),
            glm::vec3(0.8f, 0.2f, 0.8f)
        };

        for (int z = 0; z < map.height; ++z) {
            for (int x = 0; x < map.width; ++x) {
                char c = map.grid[z][x];

                glBindVertexArray(floorVAO);

                glm::mat4 fmodel(1.0f);
                fmodel = glm::translate(fmodel, glm::vec3(x + 0.5f, 0.0f, z + 0.5f));
                fmodel = glm::scale(fmodel, glm::vec3(0.5f, 1.0f, 0.5f));

                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(fmodel));

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, floorTex);
                glUniform1i(locUseTexture, 1);
                glUniform3f(locColor, 1.0f, 1.0f, 1.0f); // no tint

                glDrawArrays(GL_TRIANGLES, 0, 6);

                glUniform1i(locUseTexture, 0);

                if (c == 'W') {
                    glBindVertexArray(cubeVAO);

                    glm::mat4 model(1.0f);
                    model = glm::translate(model, glm::vec3(x + 0.5f, 0.5f, z + 0.5f));
                    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, wallTex);
                    glUniform1i(locUseTexture, 1);
                    glUniform3f(locColor, 1.0f, 1.0f, 1.0f);

                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                else if (c >= 'A' && c <= 'E') {
                    int doorIndex = c - 'A';
                    glm::vec2 doorPos(x + 0.5f, z + 0.5f);
                    glm::vec2 playerPos(cam.pos.x, cam.pos.z);
                    float ddx = playerPos.x - doorPos.x;
                    float ddz = playerPos.y - doorPos.y;

                    
                    const float unlockRadius = 1.1f;
                    float distSq = ddx * ddx + ddz * ddz;

                    if (hasKey[doorIndex] && distSq < unlockRadius * unlockRadius) {
                        map.grid[z][x] = '.';
                        hasKey[doorIndex] = false;

                        if (currentKeyIndex == doorIndex) {
                            currentKeyIndex = -1;
                            for (int k = 0; k < 5; ++k) {
                                if (hasKey[k]) {
                                    currentKeyIndex = k;
                                    break;
                                }
                            }
                        }
                        continue;
                    }

                    glBindVertexArray(doorVAO);

                    glm::mat4 model(1.0f);
                    model = glm::translate(model, glm::vec3(x + 0.5f, 0.5f, z + 0.5f));
                    model = glm::scale(model, glm::vec3(0.8f));

                    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

                    glUniform3f(locColor,
                                doorColors[doorIndex].x,
                                doorColors[doorIndex].y,
                                doorColors[doorIndex].z);

                    glUniform1i(locUseTexture, 0);
                    glDrawArrays(GL_TRIANGLES, 0, doorVertexCount);
                }
                else if (c >= 'a' && c <= 'e') {
                    glBindVertexArray(keyVAO);
                    glm::mat4 model(1.0f);
                    model = glm::translate(model, glm::vec3(x + 0.5f, 0.2f, z + 0.5f));
                    model = glm::rotate(model, goalAngle, glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(0.02f));
                    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                    int keyIndex = c - 'a';
                    glUniform3f(locColor,
                                doorColors[keyIndex].x,
                                doorColors[keyIndex].y,
                                doorColors[keyIndex].z);
                    glUniform1i(locUseTexture, 0); 
                    glDrawArrays(GL_TRIANGLES, 0, keyVertexCount);
                }
                else if (c == 'G') {
                    glBindVertexArray(goalVAO);

                    glm::mat4 model(1.0f);
                    model = glm::translate(model, glm::vec3(x + 0.5f, 0.4f, z + 0.5f));
                    model = glm::rotate(model, goalAngle, glm::vec3(0.0f, 1.0f, 1.0f));
                    model = glm::rotate(model, goalAngle, glm::vec3(1.0f, 0.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(0.5f));

                    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

                    glUniform3f(locColor, 1.0f, 0.84f, 0.0f);
                    glUniform1i(locUseTexture, 0); 

                    glDrawArrays(GL_TRIANGLES, 0, goalVertexCount);
                }
            }
        }


        if (currentKeyIndex >= 0 && currentKeyIndex < 5) {
            glBindVertexArray(keyVAO);

            glm::vec3 up(0.0f, 1.0f, 0.0f);

           
            glm::vec3 hudPos = cam.pos
                             + forward * 0.8f   
                             - up * 0.5f;      

            glm::mat4 model(1.0f);
            model = glm::translate(model, hudPos);
            model = glm::rotate(model, cam.yaw + glm::radians(90.0f),
                                glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.03f));

            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

            glUniform1i(locUseTexture, 0);
            glUniform3f(locColor,
                        doorColors[currentKeyIndex].x,
                        doorColors[currentKeyIndex].y,
                        doorColors[currentKeyIndex].z);

            glDrawArrays(GL_TRIANGLES, 0, keyVertexCount);
        }

        SDL_GL_SwapWindow(window);
    }

    glDeleteTextures(1, &wallTex);
    glDeleteTextures(1, &floorTex);
    glDeleteTextures(1, &skyTex);

    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &doorVBO);
    glDeleteVertexArrays(1, &doorVAO);
    glDeleteBuffers(1, &goalVBO);
    glDeleteVertexArrays(1, &goalVAO);
    glDeleteBuffers(1, &keyVBO);
    glDeleteVertexArrays(1, &keyVAO);
    glDeleteProgram(program);

    SDL_GL_DestroyContext(glctx);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
