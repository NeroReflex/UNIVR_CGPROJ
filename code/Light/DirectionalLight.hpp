#pragma once

#include "Light.hpp"

class DirectionalLight : public Light {
public:
    DirectionalLight() = delete;

    DirectionalLight(
        const glm::vec3& color,
        glm::float32 intensity,
        const glm::vec3& direction
    ) noexcept;

    void setDirection(const glm::vec3& direction) noexcept;

    glm::vec3 getDirection(void) const noexcept;

    

private:
    glm::vec3 m_direction;
};
