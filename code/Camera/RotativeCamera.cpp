#include "RotativeCamera.hpp"

RotativeCamera::RotativeCamera(
	const glm::float32& nearpln,
	const glm::float32& farpln,
	const glm::vec3& head,
	const glm::float32& horizAngle,
	const glm::float32& vertAngle
) noexcept
	: Camera(nearpln, farpln, head), horizontalAngle(horizAngle), verticalAngle(vertAngle) {}

void RotativeCamera::setHorizontalAngle(const glm::float32 &horizAngle) noexcept {
    horizontalAngle = horizAngle;
}

glm::float32 RotativeCamera::getHorizontalAngle() const noexcept {
    return horizontalAngle;
}

void RotativeCamera::setVerticalAngle(const glm::float32 &vertAngle) noexcept {
    verticalAngle = vertAngle;
}

glm::float32 RotativeCamera::getVerticalAngle() const noexcept {
    return verticalAngle;
}

void RotativeCamera::applyHorizontalRotation(glm::float32 amount) noexcept {
	horizontalAngle += amount;
}

void RotativeCamera::applyVerticalRotation(glm::float32 amount) noexcept {
	verticalAngle += amount;
}
