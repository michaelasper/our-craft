#include "menger.h"
#include <iostream>
using namespace std;

namespace {
const int kMinLevel = 0;
const int kMaxLevel = 4;
};  // namespace

Menger::Menger() {
    // Add additional initialization if you like
}

Menger::~Menger() {}

void Menger::set_nesting_level(int level) {
    nesting_level_ = level;
    dirty_ = true;
}

bool Menger::is_dirty() const { return dirty_; }

void Menger::set_clean() { dirty_ = false; }

// FIXME generate Menger sponge geometry
void Menger::generate_geometry(std::vector<glm::vec4>& obj_vertices,
                               std::vector<glm::uvec3>& obj_faces) const {
    obj_vertices.clear();
    obj_faces.clear();
    float edge_length = 1.0;
    float minx = -0.5f;
    float miny = -0.5f;
    float minz = -0.5f;
    float maxx = -0.5f + 1.0;
    float maxy = -0.5f + 1.0;
    float maxz = -0.5f + 1.0;
    int faces[1];
    faces[0] = 0;
    // Draw cube
    if (nesting_level_ == 0) {
        create_cube(obj_vertices, obj_faces, minx, miny, minz, maxx, maxy, maxz,
                    faces);
    } else {
        // Recursively construct the menger sponge
        menger_recursion(obj_vertices, obj_faces, minx, miny, minz,
                         edge_length / 3, nesting_level_, faces);
    }
}

void Menger::menger_recursion(std::vector<glm::vec4>& obj_vertices,
                              std::vector<glm::uvec3>& obj_faces, float minx,
                              float miny, float minz, float edge_length,
                              int depth, int* faces) const {
    float x = 0.0;
    float y = 0.0;
    float z = 0.0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                // (0,0,0) starts at (-minx, -miny, -minz),
                // This is true when only 1 of i, j, k is the value 1. We do not
                // draw the cube when this is false
                if (i % 2 + j % 2 + k % 2 < 2) {
                    x = minx + i * edge_length;
                    y = miny + j * edge_length;
                    z = minz + k * edge_length;
                    if (depth > 1)
                        menger_recursion(obj_vertices, obj_faces, x, y, z,
                                         edge_length / 3, depth - 1, faces);
                    else
                        create_cube(obj_vertices, obj_faces, x, y, z,
                                    x + edge_length, y + edge_length,
                                    z + edge_length, faces);
                }
            }
        }
    }
}

void Menger::create_cube(std::vector<glm::vec4>& obj_vertices,
                         std::vector<glm::uvec3>& obj_faces, float minx,
                         float miny, float minz, float maxx, float maxy,
                         float maxz, int* faces) const {
    // Vertices of each triangle
    int face = faces[0];
	
    // The eight vertices
	glm::vec4 point1 = glm::vec4(minx, miny, minz, 1.0f);
	glm::vec4 point2 = glm::vec4(minx, miny, maxz, 1.0f);
	glm::vec4 point3 = glm::vec4(minx, maxy, minz, 1.0f);
	glm::vec4 point4 = glm::vec4(minx, maxy, maxz, 1.0f);
	glm::vec4 point5 = glm::vec4(maxx, miny, minz, 1.0f);
	glm::vec4 point6 = glm::vec4(maxx, miny, maxz, 1.0f);
	glm::vec4 point7 = glm::vec4(maxx, maxy, minz, 1.0f);
	glm::vec4 point8 = glm::vec4(maxx, maxy, maxz, 1.0f);

    // Back Face
	// Triangle 1
	obj_vertices.push_back(point1);
	obj_vertices.push_back(point5);
	obj_vertices.push_back(point7);

	// Triangle 2
	obj_vertices.push_back(point7);
	obj_vertices.push_back(point3);
	obj_vertices.push_back(point1);

	obj_faces.push_back(glm::uvec3(face, face + 1, face + 2));
	obj_faces.push_back(glm::uvec3(face + 3, face + 4, face + 5));

    // Front Face
    // Triangle 3
    obj_vertices.push_back(point2);
    obj_vertices.push_back(point4);
    obj_vertices.push_back(point6);
    // Triangle 4
    obj_vertices.push_back(point4);
    obj_vertices.push_back(point8);
    obj_vertices.push_back(point6);

	obj_faces.push_back(glm::uvec3(face + 6, face + 7, face + 8));
	obj_faces.push_back(glm::uvec3(face + 9, face + 10, face + 11));

    // Right Face
    // Triangle 5
    obj_vertices.push_back(point5);
    obj_vertices.push_back(point6);
    obj_vertices.push_back(point7);

    // Triangle 6
    obj_vertices.push_back(point8);
    obj_vertices.push_back(point7);
    obj_vertices.push_back(point6);

	obj_faces.push_back(glm::uvec3(face + 12, face + 13, face + 14));
	obj_faces.push_back(glm::uvec3(face + 15, face + 16, face + 17));

    // Left Face
    // Triangle 7
    obj_vertices.push_back(point1);
    obj_vertices.push_back(point3);
    obj_vertices.push_back(point4);
    // Triangle q
    obj_vertices.push_back(point4);
    obj_vertices.push_back(point2);
    obj_vertices.push_back(point1);

	obj_faces.push_back(glm::uvec3(face + 18, face + 19, face + 20));
	obj_faces.push_back(glm::uvec3(face + 21, face + 22, face + 23));
 
    // Top Face
    // Triangle 9
    obj_vertices.push_back(point4);
    obj_vertices.push_back(point3);
    obj_vertices.push_back(point7);
    // Triangle 10
    obj_vertices.push_back(point8);
    obj_vertices.push_back(point4);
    obj_vertices.push_back(point7);
	
	obj_faces.push_back(glm::uvec3(face + 24, face + 25, face + 26));
	obj_faces.push_back(glm::uvec3(face + 27, face + 28, face + 29));

    // Bottom Face
    // Triangle 11
    obj_vertices.push_back(point5);
    obj_vertices.push_back(point1);
    obj_vertices.push_back(point2);
    // Triangle 12
    obj_vertices.push_back(point2);
    obj_vertices.push_back(point6);
    obj_vertices.push_back(point5);

	obj_faces.push_back(glm::uvec3(face + 30, face + 31, face + 32));
	obj_faces.push_back(glm::uvec3(face + 33, face + 34, face + 35));

    faces[0] += 36;
};