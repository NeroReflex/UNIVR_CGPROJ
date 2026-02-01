#pragma once

#include <glm/glm.hpp>
#include "Light.hpp"

class AmbientLight : public Light {
public:
    AmbientLight() = delete;

    AmbientLight(
        const glm::vec3& color,
        glm::float32 intensity
    ) noexcept;

};
