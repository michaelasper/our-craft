#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace {
float pan_speed = 0.1f;
float roll_speed = 0.1f;
float rotation_speed = 0.05f;
float zoom_speed = 0.1f;
};  // namespace

void Camera::rotate(double dx, double dy) {
    float yaw = atan(dx / camera_distance_) * rotation_speed;
    float pitch = atan(dy / camera_distance_) * rotation_speed;

    glm::mat4 matPitch = glm::translate(-pos_);
    glm::mat4 matYaw = glm::translate(-pos_);

    matPitch = glm::rotate(matPitch, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    look_ = glm::vec3(matPitch * glm::vec4(look_, 0.0f));

    matYaw = glm::rotate(matYaw, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    look_ = glm::vec3(matYaw * glm::vec4(look_, 0.0f));
    up_ = glm::vec3(matYaw * glm::vec4(up_, 0.0f));
    look_ = glm::normalize(look_);
    up_ = glm::normalize(up_);
}

void Camera::move(Camera::Direction dir) {
    glm::vec3 forward(view[0][2], view[1][2], view[2][2]);
    glm::vec3 strafe(view[0][0], view[1][0], view[2][0]);

    float deltaZ = 0.0f, deltaX = 0.0f, deltaY = 0.0f, speed = 0.0f;
    switch (dir) {
        case Camera::Direction::FORWARD: {
            deltaZ = -1.0f;
            speed = zoom_speed;
            break;
        }
        case Camera::Direction::BACKWARD: {
            deltaZ = 1.0f;
            speed = zoom_speed;
            break;
        }
        case Camera::Direction::LEFT: {
            deltaX = -1.0f;
            speed = pan_speed;
            break;
        }
        case Camera::Direction::RIGHT: {
            deltaX = 1.0f;
            speed = pan_speed;
            break;
        }
        case Camera::Direction::UP: {
            deltaY = 1.0f;
            speed = pan_speed;
            break;
        }
        case Camera::Direction::DOWN: {
            deltaY = -1.0f;
            speed = pan_speed;
            break;
        }
        default:
            break;
    }

    pos_ += (deltaZ * forward + deltaX * strafe + deltaY * up_) * speed;
}

void Camera::lookAt(glm::dvec3 eye, glm::dvec3 look, glm::dvec3 up) {
    glm::dvec3 left, y, forward;

    forward = -look_;
    left = glm::normalize(glm::cross(up, forward));
    y = glm::cross(forward, left);

    view[0][0] = left.x;
    view[1][0] = left.y;
    view[2][0] = left.z;
    view[3][0] = glm::dot(-left, eye);
    view[0][1] = y.x;
    view[1][1] = y.y;
    view[2][1] = y.z;
    view[3][1] = glm::dot(-y, eye);
    view[0][2] = forward.x;
    view[1][2] = forward.y;
    view[2][2] = forward.z;
    view[3][2] = glm::dot(-forward, eye);
    view[0][3] = 0.0;
    view[1][3] = 0.0;
    view[2][3] = 0.0;
    view[3][3] = 1.0;
}

// FIxME: Calculate the view matrix
glm::mat4 Camera::get_view_matrix() const { return view; }
