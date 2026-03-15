#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum CameraMovement {
    FORWARD,
    BACKWARD,
};

class Camera {
public:
    static Camera& getInstance() {
        if (instance == nullptr) {
            instance = new Camera();
        }
        return *instance;
    }

    // Delete copy constructor and assignment operator
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f;
    float pitch = 0.0f;

    float movementSpeed = 2.5f;
    float mouseSensitivity = 0.1f;
    float zoom = 45.0f;

    glm::vec3 initObjectPos = glm::vec3(0.0f, 0.0f, -3.0f);
    glm::mat4 projection = glm::perspective(glm::radians(zoom), (float)720 / (float)480, 0.1f, 100.0f);

    void setProjection(int width, int height){
        projection = glm::perspective(glm::radians(zoom), (float)width / (float)height, 0.1f, 100.0f);
    }

    void resetCamera() {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        yaw = -90.0f;
        pitch = 0.0f;
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    void processKeyboard(CameraMovement direction, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        if (direction == FORWARD)
            position += front * velocity;
        if (direction == BACKWARD)
            position -= front * velocity;
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        if (constrainPitch) {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        updateCameraVectors();
    }

private:
    inline static Camera* instance = nullptr; 

    Camera(){
        updateCameraVectors();
    }

    void updateCameraVectors() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);
        
        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }
};
