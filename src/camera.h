#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>

class Camera {
   public:
    enum Direction { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };
    float deltaTime = 0.0f;  // Time between current frame and last frame
    float lastFrame = 0.0f;  // Time of last frame
    glm::mat4 get_view_matrix() const;
    void lookAt(glm::dvec3 eye, glm::dvec3 look, glm::dvec3 up);
    void update() { lookAt(pos_, look_, up_); };
    void physics(float time_delta, std::vector<glm::vec3>& cubes);
    void move(Camera::Direction dir);
    void rotate(double dx, double dy);
    void walk(int direction);
    void strafe(int direction);
    void jump();
    glm::vec3 getPos() { return pos_; }

    // FIXME: add functions to manipulate camera objects.
   private:
    float camera_distance_ = 5.0;
    glm::vec3 look_ = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 pos_ = glm::vec3(0.0f, 0.0f, camera_distance_);
    glm::mat4 view = glm::mat4(1.0);
    glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    // Note: you may need additional member variables
};

#endif
