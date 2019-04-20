#include "camera.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <vector>

namespace {
float pan_speed = 0.5f;
float roll_speed = 0.5f;
float rotation_speed = 0.05f;
float zoom_speed = 0.5f;
float gravity = -.98f;
};  // namespace

float cameraHeight = 1.75f;
float cameraRadius = 0.5f;
float error = 0.1;

int collide(const glm::vec3& pos, const glm::vec3& cube) {
    int result = 0;

    float camera_y_min = pos.y - cameraHeight;
    float camera_y_max = pos.y;

    float camera_x_min = pos.x - cameraRadius;
    float camera_x_max = pos.x + cameraRadius;

    float camera_z_min = pos.z - cameraRadius;
    float camera_z_max = pos.z + cameraRadius;

    float cube_x_min = cube.x;
    float cube_x_max = cube.x + 1.0;
    float cube_y_min = cube.y;
    float cube_y_max = cube.y + 1.0;
    float cube_z_min = cube.z;
    float cube_z_max = cube.z + 1.0;

    bool cross_y_max =
        (camera_y_min < (cube_y_max + error)) && (camera_y_max > cube_y_max);
    bool cross_y_min =
        (camera_y_max > (cube_y_min - error)) && (camera_y_min < cube_y_min);
    bool cross_x_max = (camera_x_min < (cube_x_max - error)) &&
                       ((camera_x_max - error) > cube_x_max);
    bool cross_z_max = (camera_z_min < (cube_z_max - error)) &&
                       ((camera_z_max - error) > cube_z_max);
    bool cross_x_min = ((camera_x_max - error) > cube_x_min) &&
                       (camera_x_min < (cube_x_min - error));
    bool cross_z_min = ((camera_z_max - error) > cube_z_min) &&
                       (camera_z_min < (cube_z_min - error));

    bool boundsY = (camera_y_min < cube_y_max && camera_y_min > cube_y_min) ||
                   (camera_y_max < cube_y_max && camera_y_max > cube_y_min) ||
                   (camera_y_min > cube_y_min && camera_y_max < cube_y_max);
    bool boundsX = (camera_x_min < cube_x_max && camera_x_min > cube_x_min) ||
                   (camera_x_max < cube_x_max && camera_x_max > cube_x_min) ||
                   (camera_x_max < cube_x_max && camera_x_min > cube_x_min);
    bool boundsZ = (camera_z_min < cube_z_max && camera_z_min > cube_z_min) ||
                   (camera_z_max < cube_z_max && camera_z_max > cube_z_min) ||
                   (camera_z_max < cube_z_max && camera_z_min > cube_z_min);

    if (cross_y_min && boundsX && boundsZ) {
        result = result | 1;
    }
    if (cross_y_max && boundsX && boundsZ) {
        result = result | 2;
    }
    if (cross_x_max && boundsY && boundsZ) {
        result = result | 4;
    }
    if (cross_x_min && boundsY && boundsZ) {
        result = result | 8;
    }
    if (cross_z_max && boundsY && boundsX) {
        result = result | 16;
    }
    if (cross_z_min && boundsY && boundsX) {
        result = result | 32;
    }

    return result;
}

void Camera::walk(int direction) {
    auto temp = (((float)direction) * look_);
    temp.y = 0;
    this->velocity += temp;
}

void Camera::strafe(int direction) {
    auto right = cross(up_, -look_);
    this->velocity += (((float)direction) * right);
}

void Camera::jump() { 
    if(this->velocity.y == 0) this->velocity += glm::vec3(0.0f, 20.0f, 0.0f); 
    // this->velocity.y = std::max(20.0f, velocity.y);
    
}

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

void Camera::physics(float time_delta, std::vector<glm::vec3>& cubes) {
    this->velocity += glm::vec3(0.0, gravity, 0.0);

    this->velocity *= pow(0.001, time_delta);
    if (glm::length(this->velocity) < 0.05) this->velocity = glm::vec3(0.0);

    std::vector<glm::vec3> collisions;

    float coarseRadius =
        1.5 + sqrt(cameraRadius * cameraRadius + cameraHeight * cameraHeight);
    for (const auto& cube : cubes) {
        if (glm::length(cube - this->pos_) < coarseRadius) {
            collisions.push_back(cube);
        }
    }
    int collision = 0;
    for (const auto& c : collisions) {
        int result = collide(this->pos_, c);

        if (result & 2) {
            // if (this->velocity.y < -10) {
            //     this->pos_.y = c.y + cameraHeight + 1.0 + error / 2;
            // }
            this->velocity.y = std::max(0.0f, this->velocity.y);
            collision = collision | 2;
        }
        if (result & 1) {
            this->velocity.y = std::min(0.0f, this->velocity.y);
            collision = collision | 1;
        }
        if (result & 4) {
            this->velocity.x = std::max(0.0f, this->velocity.x);
            collision = collision | 4;
        }
        if (result & 8) {
            this->velocity.x = std::min(0.0f, this->velocity.x);
            collision = collision | 8;
        }
        if (result & 16) {
            this->velocity.z = std::max(0.0f, this->velocity.z);
            collision = collision | 2;
        }
        if (result & 32) {
            this->velocity.z = std::min(0.0f, this->velocity.z);
            collision = collision | 2;
        }
    }

    this->pos_ += time_delta * this->velocity;
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
