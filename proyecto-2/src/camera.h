#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up;

    float yaw;
    float pitch;

    float movement_speed;
    float mouse_sensitivity;
    float zoom;

    Camera(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up_dir = glm::vec3(0.0f, 1.0f, 0.0f), float y = -90.0f, float p = 0.0f)
        : front(glm::vec3(0.0f, 0.0f, -1.0f)), movement_speed(2.5f), mouse_sensitivity(0.1f), zoom(45.0f) {
        position = pos;
        world_up = up_dir;
        yaw = y;
        pitch = p;
        update_camera_vectors();
    }

    glm::mat4 get_view_matrix() {
        return glm::lookAt(position, position + front, up);
    }

    void process_keyboard(Camera_Movement direction, float delta_time) {
        float velocity = movement_speed * delta_time;
        if (direction == FORWARD)
            position += front * velocity;
        if (direction == BACKWARD)
            position -= front * velocity;
        if (direction == LEFT)
            position -= right * velocity;
        if (direction == RIGHT)
            position += right * velocity;
    }

private:
    void update_camera_vectors() {
        glm::vec3 new_front;
        new_front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        new_front.y = sin(glm::radians(pitch));
        new_front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(new_front);
        right = glm::normalize(glm::cross(front, world_up));
        up = glm::normalize(glm::cross(right, front));
    }
};
