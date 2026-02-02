#pragma once

#include "Light.hpp"

class ConeLight : public Light {
public:
    ConeLight() = delete;

    ConeLight(
        const glm::vec3& color,
        glm::float32 intensity,
        const glm::vec3& position,
        const glm::vec3& direction,
        glm::float32 angle_radians = glm::radians(15.0f),
        glm::float32 z_near = 1.0f,
        glm::float32 z_far = 10000.0f,
        glm::float32 inner_ratio = 0.8f,
        glm::float32 constant = 1.0f,
        glm::float32 linear = 0.09f,
        glm::float32 quadratic = 0.032f
    ) noexcept;

    void setDirection(const glm::vec3& direction) noexcept;

    glm::vec3 getDirection(void) const noexcept;

    void setPosition(const glm::vec3& position) noexcept;

    glm::vec3 getPosition(void) const noexcept;


    // Angle is stored in radians. API is explicit about radians.
    void setAngleRadians(glm::float32 radians) noexcept;
    glm::float32 getAngleRadians() const noexcept;

    // Depth range for shadowmap rendering (zNear/zFar)
    void setZNear(glm::float32 znear) noexcept;
    glm::float32 getZNear() const noexcept;

    void setZFar(glm::float32 zfar) noexcept;
    glm::float32 getZFar() const noexcept;

    // inner cone ratio (inner cone = outerHalf * inner_ratio)
    void setInnerRatio(glm::float32 r) noexcept;
    glm::float32 getInnerRatio() const noexcept;

    // attenuation constants
    void setConstant(glm::float32 c) noexcept;
    glm::float32 getConstant() const noexcept;

    void setLinear(glm::float32 l) noexcept;
    glm::float32 getLinear() const noexcept;

    void setQuadratic(glm::float32 q) noexcept;
    glm::float32 getQuadratic() const noexcept;

    

private:
    glm::vec3 m_position;
    glm::vec3 m_direction;
    glm::float32 m_angle;
    glm::float32 m_znear;
    glm::float32 m_zfar;
    glm::float32 m_inner_ratio;
    glm::float32 m_constant;
    glm::float32 m_linear;
    glm::float32 m_quadratic;
};
