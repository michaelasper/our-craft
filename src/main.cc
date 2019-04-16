#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
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
#include "menger.h"

int window_width = 800, window_height = 600;

// VBO and VAO descriptors.
enum { kVertexBuffer, kIndexBuffer, kNumVbos };

// These are our VAOs.
enum { kGeometryVao, kFloorVao, kNumVaos };

GLuint g_array_objects[kNumVaos];  // This will store the VAO descriptors.
GLuint g_buffer_objects[kNumVaos]
                       [kNumVbos];  // These will store VBO descriptors.

// C++ 11 String Literal
// See http://en.cppreference.com/w/cpp/language/string_literal
const char* vertex_shader =
    R"zzz(#version 330 core
in vec4 vertex_position;

uniform mat4 view;
uniform vec4 light_position;

out vec4 vs_light_direction;
out vec4 vs_world_pos;
void main()
{
    vs_world_pos = vertex_position;
	gl_Position = view * vertex_position;
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

in vec4 vs_world_pos[];


out vec4 light_direction;
out vec4 world_position;
void main()
{

	vec3 w_p1 = vec3(vs_world_pos[0]);
    vec3 w_p2 = vec3(vs_world_pos[1]);
    vec3 w_p3 = vec3(vs_world_pos[2]);
    world_norm = vec4(normalize(cross(w_p2 - w_p1, w_p3 - w_p1)), 0.0);

	vec3 p1 = gl_in[0].gl_Position.xyz;
	vec3 p2 = gl_in[1].gl_Position.xyz;
	vec3 p3 = gl_in[2].gl_Position.xyz;

	vec3 edge1 = p2 - p1;
	vec3 edge2 = p3 - p1;

	normal = vec4(normalize(-cross(edge1,edge2)), 0.0);

	for (int n = 0; n < gl_in.length(); n++) {
		light_direction = vs_light_direction[n];
		world_position = vs_world_pos[n];
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
in vec4 light_direction;
out vec4 fragment_color;
void main()
{
    vec4 color = vec4 (normal.x, normal.y,normal.z, 0.f);
    vec4 base_col = clamp(world_norm * world_norm, 0.0, 1.0);
    float intensity = clamp(-dot(light_direction, color), 0.0, 1.0);
    fragment_color = clamp(intensity * base_col, 0.0, 1.0);
    fragment_color.a = 1.0;
}
)zzz";

// FIXME: Implement shader effects with an alternative shader.
const char* floor_fragment_shader =
    R"zzz(#version 330 core
flat in vec4 normal;
in vec4 light_direction;
in vec4 world_position;
out vec4 fragment_color;
void main()
{
	vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
	if (int(floor(world_position[0])+floor(world_position[2])) % 2 == 0) {
		color = vec4(1.0, 1.0, 1.0, 1.0);
	}
	float dot_nl = dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	fragment_color = clamp(dot_nl * color, 0.0, 1.0);
}
)zzz";

void CreateTriangle(std::vector<glm::vec4>& vertices,
                    std::vector<glm::uvec3>& indices) {
    vertices.push_back(glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f));
    vertices.push_back(glm::vec4(0.5f, -0.5f, -0.5f, 1.0f));
    vertices.push_back(glm::vec4(0.0f, 0.5f, -0.5f, 1.0f));
    indices.push_back(glm::uvec3(0, 1, 2));
}

void createFloor(std::vector<glm::vec4>& vertices,
                 std::vector<glm::uvec3>& indices, float height, float width) {
    vertices.push_back(glm::vec4(0.0f, height, 0.0f, 1.0f));
    vertices.push_back(glm::vec4(-2000.0f, height, -2000.0f, 0.0f));
    vertices.push_back(glm::vec4(-2000.0f, height, 2000.0f, 0.0f));
    vertices.push_back(glm::vec4(2000.0f, height, 2000.0f, 0.0f));
    vertices.push_back(glm::vec4(2000.0f, height, -2000.0f, 0.0f));
    indices.push_back(glm::uvec3(0, 1, 2));
    indices.push_back(glm::uvec3(0, 2, 3));
    indices.push_back(glm::uvec3(0, 3, 4));
    indices.push_back(glm::uvec3(0, 4, 1));
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

std::shared_ptr<Menger> g_menger;
Camera g_camera;
bool g_save_geo = false;

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

    } else if (key == GLFW_KEY_W && action != GLFW_RELEASE) {
        // FIXME: WASD
        g_camera.move(Camera::Direction::FORWARD);
    } else if (key == GLFW_KEY_S && action != GLFW_RELEASE) {
        g_camera.move(Camera::Direction::BACKWARD);

    } else if (key == GLFW_KEY_A && action != GLFW_RELEASE) {
        g_camera.move(Camera::Direction::LEFT);

    } else if (key == GLFW_KEY_D && action != GLFW_RELEASE) {
        g_camera.move(Camera::Direction::RIGHT);
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
    if (!g_menger) return;  // 0-4 only available in Menger mode.
    if (key == GLFW_KEY_0 && action != GLFW_RELEASE) {
        // FIXME: Change nesting level of g_menger
        // Note: GLFW_KEY_0 - 4 may not be continuous.
        g_menger->set_nesting_level(0);
    } else if (key == GLFW_KEY_1 && action != GLFW_RELEASE) {
        g_menger->set_nesting_level(1);
    } else if (key == GLFW_KEY_2 && action != GLFW_RELEASE) {
        g_menger->set_nesting_level(2);
    } else if (key == GLFW_KEY_3 && action != GLFW_RELEASE) {
        g_menger->set_nesting_level(3);
    } else if (key == GLFW_KEY_4 && action != GLFW_RELEASE) {
        g_menger->set_nesting_level(4);
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
    std::string window_title = "Menger";
    if (!glfwInit()) exit(EXIT_FAILURE);
    g_menger = std::make_shared<Menger>();
    glfwSetErrorCallback(ErrorCallback);

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

    std::vector<glm::vec4> obj_vertices;
    std::vector<glm::uvec3> obj_faces;

    // FIXME: Create the geometry from a Menger object.
    CreateTriangle(obj_vertices, obj_faces);

    g_menger->set_nesting_level(1);

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
    CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
                                sizeof(float) * obj_vertices.size() * 4,
                                obj_vertices.data(), GL_STATIC_DRAW));
    CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
    CHECK_GL_ERROR(glEnableVertexAttribArray(0));

    // Setup element array buffer.
    CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                g_buffer_objects[kGeometryVao][kIndexBuffer]));
    CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                sizeof(uint32_t) * obj_faces.size() * 3,
                                obj_faces.data(), GL_STATIC_DRAW));

    /*
     * By far, the geometry is loaded into g_buffer_objects[kGeometryVao][*].
     * These buffers are binded to g_array_objects[kGeometryVao]
     */

    // FIXME: load the floor into g_buffer_objects[kFloorVao][*],
    //        and bind these VBO to g_array_objects[kFloorVao]

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

    // Setup fragment shader for the floor
    GLuint floor_fragment_shader_id = 0;
    const char* floor_fragment_source_pointer = floor_fragment_shader;
    CHECK_GL_ERROR(floor_fragment_shader_id =
                       glCreateShader(GL_FRAGMENT_SHADER));
    CHECK_GL_ERROR(glShaderSource(floor_fragment_shader_id, 1,
                                  &floor_fragment_source_pointer, nullptr));
    glCompileShader(floor_fragment_shader_id);
    CHECK_GL_SHADER_ERROR(floor_fragment_shader_id);

    // FIXME: Setup another program for the floor, and get its locations.
    // Note: you can reuse the vertex and geometry shader objects

    // // Setup vertex data in a VBO.

    // // NOTE: We do not send anything right now, we just describe it to

    // // Setup element array buffer.

    std::vector<glm::vec4> floor_vertices;
    std::vector<glm::uvec3> floor_faces;
    createFloor(floor_vertices, floor_faces, -2.0f, 10.0f);

    CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kFloorVao]));
    CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kFloorVao][0]));
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER,
                                g_buffer_objects[kFloorVao][kVertexBuffer]));

    CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
                                sizeof(float) * floor_vertices.size() * 4,
                                floor_vertices.data(), GL_STATIC_DRAW));
    CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
    CHECK_GL_ERROR(glEnableVertexAttribArray(0));

    CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                g_buffer_objects[kFloorVao][kIndexBuffer]));
    CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                sizeof(uint32_t) * floor_faces.size() * 3,
                                floor_faces.data(), GL_STATIC_DRAW));

    GLuint floor_program_id = 0;
    CHECK_GL_ERROR(floor_program_id = glCreateProgram());
    CHECK_GL_ERROR(glAttachShader(floor_program_id, vertex_shader_id));
    CHECK_GL_ERROR(glAttachShader(floor_program_id, floor_fragment_shader_id));
    CHECK_GL_ERROR(glAttachShader(floor_program_id, geometry_shader_id));

    // Bind attributes.
    CHECK_GL_ERROR(
        glBindAttribLocation(floor_program_id, 0, "vertex_position"));
    CHECK_GL_ERROR(
        glBindFragDataLocation(floor_program_id, 0, "fragment_color"));
    glLinkProgram(floor_program_id);
    CHECK_GL_PROGRAM_ERROR(floor_program_id);

    // Get the uniform locations.
    GLint floor_projection_matrix_location = 0;
    CHECK_GL_ERROR(floor_projection_matrix_location =
                       glGetUniformLocation(floor_program_id, "projection"));
    GLint floor_view_matrix_location = 0;
    CHECK_GL_ERROR(floor_view_matrix_location =
                       glGetUniformLocation(floor_program_id, "view"));
    GLint floor_light_position_location = 0;
    CHECK_GL_ERROR(floor_light_position_location = glGetUniformLocation(
                       floor_program_id, "light_position"));

    glm::vec4 light_position = glm::vec4(10.0f, 10.0f, 10.0f, 1.0f);
    float aspect = 0.0f;
    float theta = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        // Setup some basic window stuff.
        glfwGetFramebufferSize(window, &window_width, &window_height);
        glViewport(0, 0, window_width, window_height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LESS);

        // Switch to the Geometry VAO.
        CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

        if (g_menger && g_menger->is_dirty()) {
            g_menger->generate_geometry(obj_vertices, obj_faces);
            g_menger->set_clean();

            // FIXME: Upload your vertex data here.

            // Setup our VAO array.
            CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[0]));

            // Switch to the VAO for Geometry.
            CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

            // Generate buffer objects
            CHECK_GL_ERROR(
                glGenBuffers(kNumVbos, &g_buffer_objects[kGeometryVao][0]));

            // Setup vertex data in a VBO.
            CHECK_GL_ERROR(
                glBindBuffer(GL_ARRAY_BUFFER,
                             g_buffer_objects[kGeometryVao][kVertexBuffer]));
            // NOTE: We do not send anything right now, we just describe it to
            // OpenGL.
            CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
                                        sizeof(float) * obj_vertices.size() * 4,
                                        obj_vertices.data(), GL_STATIC_DRAW));
            CHECK_GL_ERROR(
                glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
            CHECK_GL_ERROR(glEnableVertexAttribArray(0));

            // Setup element array buffer.
            CHECK_GL_ERROR(
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                             g_buffer_objects[kGeometryVao][kIndexBuffer]));
            CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                        sizeof(uint32_t) * obj_faces.size() * 3,
                                        obj_faces.data(), GL_STATIC_DRAW));
        }

        // Save the geometry to a .obj file if needed
        if (g_save_geo) {
            SaveObj("geometry.obj", obj_vertices, obj_faces);
            g_save_geo = false;
        }

        // Compute the projection matrix.
        aspect = static_cast<float>(window_width) / window_height;
        glm::mat4 projection_matrix =
            glm::perspective(glm::radians(45.0f), aspect, 0.0001f, 1000.0f);

        // float min = -0.001f;
        // float max = 0.001f;
        // projection_matrix = glm::ortho(min * window_width, max *
        // window_width,
        //                                min * window_height, max *
        //                                window_height, 0.0001f, 1000.0f);
        // Compute the view matrix
        // FIXME: change eye and center through mouse/keyboard events.
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
        CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, obj_faces.size() * 3,
                                      GL_UNSIGNED_INT, 0));

        CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kFloorVao]));
        CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kFloorVao][0]));
        CHECK_GL_ERROR(glBindBuffer(
            GL_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kVertexBuffer]));

        CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
                                    sizeof(float) * floor_vertices.size() * 4,
                                    floor_vertices.data(), GL_STATIC_DRAW));
        CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
        CHECK_GL_ERROR(glEnableVertexAttribArray(0));

        CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                    g_buffer_objects[kFloorVao][kIndexBuffer]));
        CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                    sizeof(uint32_t) * floor_faces.size() * 3,
                                    floor_faces.data(), GL_STATIC_DRAW));
        CHECK_GL_ERROR(glUseProgram(floor_program_id));

        // // Pass uniforms in.
        CHECK_GL_ERROR(glUniformMatrix4fv(projection_matrix_location, 1,
                                          GL_FALSE, &projection_matrix[0][0]));
        CHECK_GL_ERROR(glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE,
                                          &view_matrix[0][0]));
        CHECK_GL_ERROR(
            glUniform4fv(light_position_location, 1, &light_position[0]));
        CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3,
                                      GL_UNSIGNED_INT, 0));
        // CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3,
        //                               GL_UNSIGNED_INT, 0));

        // Poll and swap.
        glfwPollEvents();
        glfwSwapBuffers(window);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
