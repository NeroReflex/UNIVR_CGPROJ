#pragma once

#include <tuple>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>

#include "Armature.hpp"
#include <glm/gtc/quaternion.hpp>

#define MAX_ANIMATION_CHANNELS 128u

#define MAX_ANIMATION_KEYS_PER_CHANNEL 64u

struct AnimationGPUChannel {
    // The index of a ArmatureGPUElement in the Armature's nodes buffer
    uint32_t armature_element_index;

    uint32_t position_key_count;
    float position_key_times[MAX_ANIMATION_KEYS_PER_CHANNEL];
    float position_key_value_x[MAX_ANIMATION_KEYS_PER_CHANNEL];
    float position_key_value_y[MAX_ANIMATION_KEYS_PER_CHANNEL];
    float position_key_value_z[MAX_ANIMATION_KEYS_PER_CHANNEL];
    
    // TODO: type of interpolation (e.g., linear, cubic, etc.)
    //glm::uint position_key_transform[MAX_ANIMATION_KEYS_PER_CHANNEL];

    uint32_t rotation_key_count;
    float rotation_key_times[MAX_ANIMATION_KEYS_PER_CHANNEL];
    float rotation_key_value_x[MAX_ANIMATION_KEYS_PER_CHANNEL];
    float rotation_key_value_y[MAX_ANIMATION_KEYS_PER_CHANNEL];
    float rotation_key_value_z[MAX_ANIMATION_KEYS_PER_CHANNEL];
    float rotation_key_value_w[MAX_ANIMATION_KEYS_PER_CHANNEL];

    uint32_t scaling_key_count;
    float scaling_key_times[MAX_ANIMATION_KEYS_PER_CHANNEL];
    float scaling_key_value_x[MAX_ANIMATION_KEYS_PER_CHANNEL];
    float scaling_key_value_y[MAX_ANIMATION_KEYS_PER_CHANNEL];
    float scaling_key_value_z[MAX_ANIMATION_KEYS_PER_CHANNEL];
};

class Animation {
public:
    Animation(
        double duration,
        double ticks_per_second,
        std::shared_ptr<Armature> armature,
        GLuint channels_buffer,
        GLuint channels_count
    ) noexcept;

    ~Animation() = default;

    Animation(const Animation&) = delete;

    Animation& operator=(const Animation&) = delete;

    static Animation* CreateAnimation(
        double duration,
        double ticks_per_second,
        std::shared_ptr<Armature> armature
    ) noexcept;

    double getDuration() const noexcept { return m_duration; }

    double getTicksPerSecond() const noexcept { return m_ticksPerSecond; }

    bool addChannel(
        std::string armature_node_name,
        const std::vector<std::tuple<double, glm::vec3>>& position_keys,
        const std::vector<std::tuple<double, glm::quat>>& rotation_keys,
        const std::vector<std::tuple<double, glm::vec3>>& scaling_keys
    ) noexcept;

    GLuint getChannelsBuffer() const noexcept { return m_channels_buffer; }

    GLuint getChannelsCount() const noexcept { return m_channels_count; }

private:
    double m_duration;

    double m_ticksPerSecond;

    std::shared_ptr<Armature> m_armature;

    GLuint m_channels_buffer;
    GLuint m_channels_count;
};
