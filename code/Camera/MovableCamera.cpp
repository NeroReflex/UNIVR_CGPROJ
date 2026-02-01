#include "MovableCamera.hpp"

MovableCamera::MovableCamera(
	const glm::vec3& position,
	const glm::float32& nearpln,
	const glm::float32& farpln,
	const glm::vec3& head
) noexcept
	: Camera(nearpln, farpln, head), position(position) {}

glm::vec3 MovableCamera::getCameraPosition() const noexcept {
    return position;
}

void MovableCamera::applyMovement(const glm::vec3& moveVect, glm::float32 amount) noexcept {
    position += moveVect * amount;
}

void MovableCamera::setCameraPosition(const glm::vec3& pos) noexcept {
    position = pos;
}
