#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

// OpenGL library includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <debuggl.h>
#include "camera.h"
#include "cube.cc"
// #include "perlin.h"
#include "terrain.h"

int window_width = 800, window_height = 600;

constexpr unsigned int cubes = 50000;

// VBO and VAO descriptors.
enum { kVertexBuffer, kIndexBuffer, kNumVbos };

// These are our VAOs.
enum { kGeometryVao, kNumVaos };

GLuint g_array_objects[kNumVaos];  // This will store the VAO descriptors.
GLuint g_buffer_objects[kNumVaos]
                       [kNumVbos];  // These will store VBO descriptors.

// C++ 11 String Literal
// See http://en.cppreference.com/w/cpp/language/string_literal
const char* vertex_shader =
    R"zzz(#version 330 core
layout(location = 0) in vec4 vertex_position;
layout(location = 1) in vec3 cube_offset;
uniform mat4 view;
uniform vec4 light_position;

out vec4 vs_light_direction;
out vec4 vs_world_pos;
out vec4 pos;

void main()
{
    pos = vertex_position + vec4(cube_offset, 0.0);
    vs_world_pos = vertex_position;
	gl_Position = view * pos;
	vs_light_direction = view * (vertex_position - light_position);
}
)zzz";

const char* geometry_shader =
    R"zzz(#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform mat4 projection;
uniform mat4 view;

in vec4 vs_light_direction[];
flat out vec4 normal;
flat out vec4 world_norm;
in vec4 pos[];
in vec4 vs_world_pos[];


out vec4 light_direction;
out vec4 world_position;
void main()
{

	vec3 side2 = (pos[2] - pos[0]).xyz;
    vec3 side1 = (pos[1] - pos[0]).xyz;
    vec3 normal3 = normalize(cross(side1,side2));
    vec4 faceNormal = vec4(normal3[0], normal3[1], normal3[2], 0.0);

	for (int n = 0; n < gl_in.length(); n++) {
		light_direction = vs_light_direction[n];
        normal = faceNormal;
		world_position = pos[n];
		gl_Position = projection  * gl_in[n].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}
)zzz";

const char* fragment_shader =
    R"zzz(#version 330 core
flat in vec4 normal;
flat in vec4 world_norm;
in vec4 world_position;
in vec4 light_direction;
uniform mat4 view;

out vec4 fragment_color;

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

float fbm(vec3 x) {
	float v = 0.0;
	float a = 0.5;
	vec3 shift = vec3(20);
	for (int i = 0; i < 3; ++i) {
		v += a * noise(x);
		x = x * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}

void main()
{

    int pixely = 4;
    int smooths = 2;
    // int smooth = 1;

    vec4 baseCol;
    if(world_position.y < .125){
        baseCol = vec4(0.1,0.4,0.8,1.0);
        pixely = 2;
        smooths = 16;
    }
    else if(world_position.y < 10.025){
        baseCol = vec4(0.3,0.8,0.15,1.0);
    }else{
        baseCol = vec4(0.7,0.7,0.7,1.0);
    }
  
    float col = noise(floor(world_position.xyz * pixely)/smooths);

    fragment_color = 0.4 * (col * baseCol) + 0.6 * (baseCol); 
    fragment_color += vec4(0.15,0.15,0.15, 0.0);

    float dot_nl = dot(normalize(light_direction), view * normalize(normal));
    dot_nl = clamp(dot_nl, 0.4, 1.0);
    fragment_color = clamp( fragment_color * dot_nl, 0.0, 0.8);
    fragment_color[3] = 1.0;
}
)zzz";

void CreateTriangle(std::vector<glm::vec4>& vertices,
                    std::vector<glm::uvec3>& indices) {
    vertices.push_back(glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f));
    vertices.push_back(glm::vec4(0.5f, -0.5f, -0.5f, 1.0f));
    vertices.push_back(glm::vec4(0.0f, 0.5f, -0.5f, 1.0f));
    indices.push_back(glm::uvec3(0, 1, 2));
}

std::string readFile(const char* filePath) {
    std::string content;
    std::ifstream fileStream(filePath, std::ios::in);

    if (!fileStream.is_open()) {
        std::cerr << "Could not read file " << filePath
                  << ". File does not exist." << std::endl;
        return "";
    }

    std::string line = "";
    while (!fileStream.eof()) {
        std::getline(fileStream, line);
        content.append(line + "\n");
    }

    fileStream.close();
    return content;
}

void SaveObj(const std::string& file, const std::vector<glm::vec4>& vertices,
             const std::vector<glm::uvec3>& indices) {
    std::ofstream ofs(file);
    for (const auto& vert : vertices)
        ofs << "v " << vert[0] << " " << vert[1] << " " << vert[2] << std::endl;
    for (const auto& index : indices)
        ofs << "f " << index[0] + 1 << " " << index[1] + 1 << " "
            << index[2] + 1 << std::endl;
}

void ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error: " << description << "\n";
}

// std::shared_ptr<Menger> g_menger;
Camera g_camera;
bool g_save_geo = false;
bool g_gravity = false;
std::random_device rd;
std::mt19937 gen(rd());
Terrain terrain(gen);

// Point cube intersect
bool intersect(glm::vec2 point, glm::vec2 cube) {
    // xmin<=x<=xmax && ymin<=y<=ymax && zmin<=z<=zmax
    float xmin = cube.x;
    float xmax = cube.x + 1;
    float zmin = cube.y - 1;
    float zmax = cube.y;

    std::cout << "point " << point[0] << " " << point[1] << std::endl;
    std::cout << "cube " << cube[0] << " " << cube[1] << std::endl;
    return xmin <= point.x && point.x <= xmax && zmin <= point.y && point.y <= zmax;
}

bool CollisionDetection() {
    glm::vec3 player_pos = g_camera.getPos();
    glm::ivec2 chunk_coords = terrain.toChunkCoords(player_pos);
    std::vector<glm::vec3> cubes = terrain.genChunkSurface(chunk_coords);

    std::cout << "player pos: " << player_pos[0] << " " << player_pos[1] << " " << player_pos[2] << std::endl;
    for(auto c : cubes) {
        auto x = c + glm::vec3(chunk_coords.x * terrain.size, 0, chunk_coords.y * terrain.size);

        // PLayer should be at same height as the intersecting cube
        if(x.y >= player_pos.y - 2 && x.y <= player_pos.y + 2 ) {
            if((x.x - player_pos.x) * (x.x - player_pos.x) + (x.z - player_pos.z) * (x.z - player_pos.z) <= 16 ) {
                std::cout << "potential intersection at " << x[0] << " " << x[1] << " " << x[2] <<std::endl;
                if(intersect(glm::vec2(player_pos.x, player_pos.z), glm::vec2(x.x, x.z))) {
                    std::cout << "intersected" << std::endl;
                    std::cout << x[0] << " " << x[1] << " " << x[2] << std::endl;
                    return true;
                }
            }
        }

    }
    return false;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action,
                 int mods) {
    // Note:
    // This is only a list of functions to implement.
    // you may want to re-organize this piece of code.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    else if (key == GLFW_KEY_S && mods == GLFW_MOD_CONTROL &&
             action == GLFW_RELEASE) {
        g_save_geo = true;
    } 
    else if (key == GLFW_KEY_F && mods == GLFW_MOD_CONTROL && 
             action == GLFW_RELEASE) {
        if(!g_gravity)
            std::cout << "Gravity turned on" << std::endl;
        else
            std::cout << "Gravity turned off" << std::endl;
        g_gravity = !g_gravity;
    } else if (key == GLFW_KEY_W && action != GLFW_RELEASE) {
        // FIXME: WASD
        if(g_gravity) {
            bool collision = CollisionDetection();
            std::cout << g_gravity << std::endl;
            if(!collision)
                g_camera.move(Camera::Direction::FORWARD);
        }
        else { 
            g_camera.move(Camera::Direction::FORWARD);
        }
    } else if (key == GLFW_KEY_S && action != GLFW_RELEASE) {
        if(g_gravity) {
            bool collision = CollisionDetection();
            std::cout << g_gravity << std::endl;
            if(!collision)
                g_camera.move(Camera::Direction::BACKWARD);
        }
        else { 
            g_camera.move(Camera::Direction::BACKWARD);
        }

    } else if (key == GLFW_KEY_A && action != GLFW_RELEASE) {
        if(g_gravity) {
            bool collision = CollisionDetection();
            std::cout << g_gravity << std::endl;
            if(!collision)
                g_camera.move(Camera::Direction::LEFT);
        }
        else {
            g_camera.move(Camera::Direction::LEFT);
        }

    } else if (key == GLFW_KEY_D && action != GLFW_RELEASE) {
        if(g_gravity) {
            bool collision = CollisionDetection();
            std::cout << g_gravity << std::endl;
            if(!collision)
                g_camera.move(Camera::Direction::RIGHT);
        }
        else { 
            g_camera.move(Camera::Direction::RIGHT);
        }
    } else if (key == GLFW_KEY_Q && action != GLFW_RELEASE) {
        exit(1);
    } else if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE) {
        // FIXME: Left Right Up and Down
    } else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE) {
    } else if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE) {
        g_camera.move(Camera::Direction::DOWN);
    } else if (key == GLFW_KEY_UP && action != GLFW_RELEASE) {
        g_camera.move(Camera::Direction::UP);
    } else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
        // FIXME: FPS mode on/off
    }
}

int g_current_button;
bool g_mouse_pressed;
double g_mouse_x;
double g_mouse_y;

void MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y) {
    if (!g_mouse_pressed) return;

    double deltaX = mouse_x - g_mouse_x;
    double deltaY = mouse_y - g_mouse_y;

    if (g_current_button == GLFW_MOUSE_BUTTON_LEFT) {
        g_camera.rotate(deltaX, deltaY);
    } else if (g_current_button == GLFW_MOUSE_BUTTON_RIGHT) {
        // FIXME: middle drag
    } else if (g_current_button == GLFW_MOUSE_BUTTON_MIDDLE) {
        // FIXME: right drag
    }

    g_mouse_x = mouse_x;
    g_mouse_y = mouse_y;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    g_mouse_pressed = (action == GLFW_PRESS);
    g_current_button = button;
}

int main(int argc, char* argv[]) {
    std::string window_title = "Minecraft";
    if (!glfwInit()) exit(EXIT_FAILURE);
    // g_menger = std::make_shared<Menger>();
    glfwSetErrorCallback(ErrorCallback);
    // std::random_device rd;
    // std::mt19937 gen(rd());
    // terrain(gen);

    // Ask an OpenGL 4.1 core profile context
    // It is required on OSX and non-NVIDIA Linux
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(window_width, window_height,
                                          &window_title[0], nullptr, nullptr);
    CHECK_SUCCESS(window != nullptr);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glfwWindowHint(GLFW_SAMPLES, 16);
    glEnable(GL_MULTISAMPLE);

    CHECK_SUCCESS(glewInit() == GLEW_OK);
    glGetError();  // clear GLEW's error for it
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MousePosCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSwapInterval(1);
    const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
    const GLubyte* version = glGetString(GL_VERSION);    // version as a string
    std::cout << "Renderer: " << renderer << "\n";
    std::cout << "OpenGL version supported:" << version << "\n";

    std::vector<glm::vec4> obj_vertices = Cube::vertices;
    std::vector<glm::uvec3> obj_faces = Cube::faces;
    std::vector<glm::vec3> offsets;
    offsets.resize(cubes);

    glm::vec4 min_bounds = glm::vec4(std::numeric_limits<float>::max());
    glm::vec4 max_bounds = glm::vec4(-std::numeric_limits<float>::max());
    for (const auto& vert : obj_vertices) {
        min_bounds = glm::min(vert, min_bounds);
        max_bounds = glm::max(vert, max_bounds);
    }
    std::cout << "min_bounds = " << glm::to_string(min_bounds) << "\n";
    std::cout << "max_bounds = " << glm::to_string(max_bounds) << "\n";

    // Setup our VAO array.
    CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[0]));

    // Switch to the VAO for Geometry.
    CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

    // Generate buffer objects
    CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kGeometryVao][0]));

    // Setup vertex data in a VBO.
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER,
                                g_buffer_objects[kGeometryVao][kVertexBuffer]));

    // NOTE: We do not send anything right now, we just describe it to OpenGL.

    int numVertices = sizeof(float) * obj_vertices.size() * 4;
    int numOffsets = sizeof(float) * offsets.size() * 3;
    CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, numVertices + numOffsets, 0,
                                GL_STATIC_DRAW));
    CHECK_GL_ERROR(
        glBufferSubData(GL_ARRAY_BUFFER, 0, numVertices, obj_vertices.data()));
    CHECK_GL_ERROR(glBufferSubData(GL_ARRAY_BUFFER, numVertices, numOffsets,
                                   offsets.data()));

    // Enable vertex positions to be passed in under location 0
    CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
    CHECK_GL_ERROR(glEnableVertexAttribArray(0));

    // Enable vertex offsets to be passed in under location 1, instanced
    CHECK_GL_ERROR(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,
                                         (void*)((uint64_t)numVertices)));
    CHECK_GL_ERROR(glEnableVertexAttribArray(1));
    CHECK_GL_ERROR(glVertexAttribDivisor(1, 1));  // Per-instance locations

    // Setup element array buffer.
    CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                g_buffer_objects[kGeometryVao][kIndexBuffer]));
    CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                sizeof(uint32_t) * obj_faces.size() * 3,
                                obj_faces.data(), GL_STATIC_DRAW));

    // Setup vertex shader.
    GLuint vertex_shader_id = 0;
    // std::string triangle_vert = readFile("../src/triangle.vert");
    // const char* vertex_source_pointer = triangle_vert.c_str();
    const char* vertex_source_pointer = vertex_shader;
    CHECK_GL_ERROR(vertex_shader_id = glCreateShader(GL_VERTEX_SHADER));
    CHECK_GL_ERROR(
        glShaderSource(vertex_shader_id, 1, &vertex_source_pointer, nullptr));
    glCompileShader(vertex_shader_id);
    CHECK_GL_SHADER_ERROR(vertex_shader_id);

    // Setup geometry shader.
    GLuint geometry_shader_id = 0;
    // std::string triangle_geom = readFile("../src/triangle.geom");
    const char* geometry_source_pointer = geometry_shader;
    CHECK_GL_ERROR(geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER));
    CHECK_GL_ERROR(glShaderSource(geometry_shader_id, 1,
                                  &geometry_source_pointer, nullptr));
    glCompileShader(geometry_shader_id);
    CHECK_GL_SHADER_ERROR(geometry_shader_id);

    // Setup fragment shader.
    GLuint fragment_shader_id = 0;
    // std::string triangle_frag = readFile("../src/triangle.frag");
    const char* fragment_source_pointer = fragment_shader;
    CHECK_GL_ERROR(fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
    CHECK_GL_ERROR(glShaderSource(fragment_shader_id, 1,
                                  &fragment_source_pointer, nullptr));
    glCompileShader(fragment_shader_id);
    CHECK_GL_SHADER_ERROR(fragment_shader_id);

    // Let's create our program.
    GLuint program_id = 0;
    CHECK_GL_ERROR(program_id = glCreateProgram());
    CHECK_GL_ERROR(glAttachShader(program_id, vertex_shader_id));
    CHECK_GL_ERROR(glAttachShader(program_id, fragment_shader_id));
    CHECK_GL_ERROR(glAttachShader(program_id, geometry_shader_id));

    // Bind attributes.
    CHECK_GL_ERROR(glBindAttribLocation(program_id, 0, "vertex_position"));
    CHECK_GL_ERROR(glBindFragDataLocation(program_id, 0, "fragment_color"));
    glLinkProgram(program_id);
    CHECK_GL_PROGRAM_ERROR(program_id);

    // Get the uniform locations.
    GLint projection_matrix_location = 0;
    CHECK_GL_ERROR(projection_matrix_location =
                       glGetUniformLocation(program_id, "projection"));
    GLint view_matrix_location = 0;
    CHECK_GL_ERROR(view_matrix_location =
                       glGetUniformLocation(program_id, "view"));
    GLint light_position_location = 0;
    CHECK_GL_ERROR(light_position_location =
                       glGetUniformLocation(program_id, "light_position"));

    // ▄▄▄▄▄▄▄▄▄▄▄  ▄    ▄  ▄         ▄
    // ▐░░░░░░░░░░░▌▐░▌  ▐░▌▐░▌       ▐░▌
    // ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌ ▐░▌ ▐░▌       ▐░▌
    // ▐░▌          ▐░▌▐░▌  ▐░▌       ▐░▌
    // ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌░▌   ▐░█▄▄▄▄▄▄▄█░▌
    // ▐░░░░░░░░░░░▌▐░░▌    ▐░░░░░░░░░░░▌
    //  ▀▀▀▀▀▀▀▀▀█░▌▐░▌░▌    ▀▀▀▀█░█▀▀▀▀
    //           ▐░▌▐░▌▐░▌       ▐░▌
    //  ▄▄▄▄▄▄▄▄▄█░▌▐░▌ ▐░▌      ▐░▌
    // ▐░░░░░░░░░░░▌▐░▌  ▐░▌     ▐░▌
    //  ▀▀▀▀▀▀▀▀▀▀▀  ▀    ▀       ▀

    glm::vec4 light_position = glm::vec4(10.0f, 10.0f, 10.0f, 1.0f);
    float aspect = 0.0f;
    float theta = 0.0f;
    glm::ivec2 prevChunk(-500, -500);
    while (!glfwWindowShouldClose(window)) {
        glm::ivec2 curChunk = terrain.toChunkCoords(g_camera.getPos());

        if (curChunk != prevChunk) {
            prevChunk = curChunk;
            offsets = terrain.getSurfaceForRender(g_camera.getPos());

            CHECK_GL_ERROR(
                glBindBuffer(GL_ARRAY_BUFFER,
                             g_buffer_objects[kGeometryVao][kVertexBuffer]));

            CHECK_GL_ERROR(glBufferSubData(
                GL_ARRAY_BUFFER, sizeof(float) * obj_vertices.size() * 4,
                sizeof(float) * cubes * 3, offsets.data()));
        }

        glfwGetFramebufferSize(window, &window_width, &window_height);
        glViewport(0, 0, window_width, window_height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LESS);

        // Switch to the Geometry VAO.
        CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

        // Compute the projection matrix.
        aspect = static_cast<float>(window_width) / window_height;
        glm::mat4 projection_matrix =
            glm::perspective(glm::radians(45.0f), aspect, 0.0001f, 1000.0f);

        g_camera.update();
        glm::mat4 view_matrix = g_camera.get_view_matrix();

        // Use our program.
        CHECK_GL_ERROR(glUseProgram(program_id));

        // Pass uniforms in.
        CHECK_GL_ERROR(glUniformMatrix4fv(projection_matrix_location, 1,
                                          GL_FALSE, &projection_matrix[0][0]));
        CHECK_GL_ERROR(glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE,
                                          &view_matrix[0][0]));
        CHECK_GL_ERROR(
            glUniform4fv(light_position_location, 1, &light_position[0]));

        // Draw our triangles.
        CHECK_GL_ERROR(glDrawElementsInstanced(
            GL_TRIANGLES, obj_faces.size() * 3, GL_UNSIGNED_INT, 0, cubes));

        // Poll and swap.
        glfwPollEvents();
        glfwSwapBuffers(window);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
