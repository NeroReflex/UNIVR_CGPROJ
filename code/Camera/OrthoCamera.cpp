#include "OrthoCamera.hpp"

OrthoCamera::OrthoCamera(
    const glm::float32& nearpln,
    const glm::float32& farpln,
    const glm::vec3& head,
    const glm::float32& orthoHeight) noexcept
    : Camera(nearpln, farpln, head), height(orthoHeight) {}

void OrthoCamera::setOrthoHeight(const glm::float32& orthoHeight) const noexcept {
    height = orthoHeight;
}

glm::float32 OrthoCamera::getOrthoHeight() const noexcept {
    return height;
}

glm::mat4 OrthoCamera::getProjectionMatrix(glm::uint32 width, glm::uint32 heightPx) const noexcept {
    // compute aspect and derive left/right/top/bottom from stored half-height
    const glm::float32 aspect = (glm::float32)width / (glm::float32)heightPx;
    const glm::float32 top = getOrthoHeight();
    const glm::float32 bottom = -top;
    const glm::float32 right = top * aspect;
    const glm::float32 left = -right;

    return glm::ortho(left, right, bottom, top, getNearPlane(), getFarPlane());
}
