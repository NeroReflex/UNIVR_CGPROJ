#pragma once

#include <glm/glm.hpp>

class Light {
public:
    Light() = delete;

    Light(const glm::vec3& color, glm::float32 intensity = 1.0f) noexcept;

    void setColor(const glm::vec3& color) noexcept;
    glm::vec3 getColor(void) const noexcept;

    void setIntensity(glm::float32 intensity) noexcept;
    glm::float32 getIntensity() const noexcept;

    glm::vec3 getColorWithIntensity(void) const noexcept;

protected:
    glm::vec3 m_color;
    glm::float32 m_intensity;
};
