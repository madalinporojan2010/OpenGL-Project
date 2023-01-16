#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;
        
        this->originalCameraFront = glm::normalize(this->cameraTarget - this->cameraPosition);
        this->originalCameraUp = cameraUp;
        this->rotate(0.0f , -90.0f);
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {        
        return glm::lookAt(this->cameraPosition, this->cameraPosition + this->cameraFrontDirection, cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        std::printf("Coords: %f %f %f\n", this->cameraPosition.x, this->cameraPosition.y, this->cameraPosition.z);
        switch (direction) {
            case MOVE_FORWARD:
                this->cameraPosition -= speed * (-this->cameraFrontDirection);
                break;
            case MOVE_BACKWARD:
                this->cameraPosition -= speed * this->cameraFrontDirection;
                break;
            case MOVE_LEFT:
                this->cameraPosition -= speed * (-this->cameraRightDirection);
                break;
            case MOVE_RIGHT:
                this->cameraPosition -= speed * this->cameraRightDirection; //why minus
                break;
        }
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        this->cameraFrontDirection = - (glm::normalize(glm::yawPitchRoll(glm::radians(-yaw), glm::radians(-pitch), glm::radians(0.0f)) * glm::vec4(this->originalCameraFront, 1.0f)));
        this->cameraRightDirection = - (glm::normalize(glm::cross(this->cameraFrontDirection, this->originalCameraUp)));
        this->cameraUpDirection = glm::cross(this->cameraFrontDirection, this->cameraRightDirection);
    }
}