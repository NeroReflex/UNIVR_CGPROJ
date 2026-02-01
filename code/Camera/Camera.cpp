#include "Camera.hpp"

const glm::vec3 Camera::HeadUp = {0.0, 1.0, 0.0};
const glm::vec3 Camera::HeadDown = {0.0, -1.0, 0.0};

Camera::Camera(const glm::float32 &nearpln, const glm::float32 &farpln, const glm::vec3 &head) noexcept
    : nearPlane(nearpln), farPlane(farpln), head(head) {}

glm::vec3 Camera::getHead() const noexcept {
    return head;
}

glm::float32 Camera::getNearPlane() const noexcept {
    return nearPlane;
}

glm::float32 Camera::getFarPlane() const noexcept {
    return farPlane;
}

glm::vec3 Camera::getOrientation() const noexcept {
    const auto verticalAngle = getVerticalAngle(), horizontalAngle = getHorizontalAngle();
    return glm::normalize(glm::vec3(
            glm::cos(verticalAngle) * glm::sin(horizontalAngle),
            glm::sin(verticalAngle),
            glm::cos(verticalAngle) * glm::cos(horizontalAngle)
    ));
}

glm::mat4 Camera::getViewMatrix() const noexcept {
    const auto glm_position = getCameraPosition();
    return glm::lookAt(
            glm_position,
            glm_position + getOrientation(),
            getHead()
    );
}
