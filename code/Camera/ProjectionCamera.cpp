#include "ProjectionCamera.hpp"

ProjectionCamera::ProjectionCamera(
	const glm::float32& nearpln,
	const glm::float32& farpln,
	const glm::vec3& head,
	const glm::float32& fovDegree
) noexcept
	: Camera(nearpln, farpln, head), fov(glm::radians(fovDegree)) {}

void ProjectionCamera::setFieldOfView(const glm::float32& fovDegree) const noexcept {
    fov = glm::radians(fovDegree);
}

glm::float32 ProjectionCamera::getFieldOfView() const noexcept {
    return fov;
}

glm::mat4 ProjectionCamera::getProjectionMatrix(glm::uint32 width, glm::uint32 height) const noexcept {
    return glm::perspective(getFieldOfView(), (glm::float32)width / height, getNearPlane(), getFarPlane());
}
