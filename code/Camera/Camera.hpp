#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera {

public:
    static const glm::vec3 HeadUp;
    static const glm::vec3 HeadDown;

    static constexpr glm::float32 defaultNearPlane = 0.0001;
    static constexpr glm::float32 defaultFarPlane = 100000.0;

    explicit Camera(
        const glm::float32& nearpln = Camera::defaultNearPlane,
        const glm::float32& farpln = Camera::defaultFarPlane,
        const glm::vec3& head = Camera::HeadUp
    ) noexcept;

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    virtual ~Camera() = default;

    /**
        * Get the position of the camera in the 3D space.
        * This is not called getPosition because if an actor is also a camera getPosition would result in wrong positioning!
        *
        * @return the camera position
        */
    virtual glm::vec3 getCameraPosition() const noexcept = 0;

    /**
        * Get the camera orientation (the direction the camera il looking at) as a normalized (length = 1) vector.
        *
        * @return a vector of length one giving the camera orientation
        */
    virtual glm::vec3 getOrientation() const noexcept;

    glm::vec3 getHead() const noexcept;

    glm::float32 getNearPlane() const noexcept;

    glm::float32 getFarPlane() const noexcept;

    virtual glm::float32 getHorizontalAngle() const noexcept = 0;

    virtual glm::float32 getVerticalAngle() const noexcept = 0;

    glm::mat4 getViewMatrix() const noexcept;

    virtual glm::mat4 getProjectionMatrix(glm::uint32 width, glm::uint32 height) const noexcept = 0;

private:
    glm::vec3 head;
    glm::float32 nearPlane;
    glm::float32 farPlane;
};
