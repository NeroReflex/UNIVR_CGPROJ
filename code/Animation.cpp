#include "Animation.hpp"

#include <iostream>

Animation::Animation(
    double duration,
    double ticks_per_second,
    std::shared_ptr<Armature> armature,
    GLuint channels_buffer,
    GLuint channels_count
) noexcept :
    m_duration(duration),
    m_ticksPerSecond(ticks_per_second),
    m_armature(armature),
    m_channels_buffer(channels_buffer),
    m_channels_count(channels_count)
{

}

bool Animation::addChannel(
    std::string armature_node_name,
    const std::vector<std::tuple<double, glm::vec3>>& position_keys,
    const std::vector<std::tuple<double, glm::quat>>& rotation_keys,
    const std::vector<std::tuple<double, glm::vec3>>& scaling_keys
) noexcept {
    assert(position_keys.size() <= MAX_ANIMATION_KEYS_PER_CHANNEL && "Exceeded maximum number of position keys per channel");
    assert(rotation_keys.size() <= MAX_ANIMATION_KEYS_PER_CHANNEL && "Exceeded maximum number of rotation keys per channel");
    assert(scaling_keys.size() <= MAX_ANIMATION_KEYS_PER_CHANNEL && "Exceeded maximum number of scaling keys per channel");
    assert(m_channels_count < MAX_ANIMATION_CHANNELS && "Exceeded maximum number of animation channels");

    AnimationGPUChannel gpu_channel = {};
    const auto armature_node_index_opt = m_armature->findArmatureNodeByName(armature_node_name);
    assert(armature_node_index_opt.has_value() && "Armature node not found in armature");

    gpu_channel.armature_element_index = armature_node_index_opt.value();
    gpu_channel.position_key_count = static_cast<uint32_t>(position_keys.size());
    for (size_t i = 0; i < position_keys.size(); ++i) {
        gpu_channel.position_key_times[i] = static_cast<float>(std::get<0>(position_keys[i]));
        gpu_channel.position_key_value_x[i] = std::get<1>(position_keys[i]).x;
        gpu_channel.position_key_value_y[i] = std::get<1>(position_keys[i]).y;
        gpu_channel.position_key_value_z[i] = std::get<1>(position_keys[i]).z;
    }

    gpu_channel.rotation_key_count = static_cast<uint32_t>(rotation_keys.size());
    for (size_t i = 0; i < rotation_keys.size(); ++i) {
        gpu_channel.rotation_key_times[i] = static_cast<float>(std::get<0>(rotation_keys[i]));
        gpu_channel.rotation_key_value_x[i] = std::get<1>(rotation_keys[i]).x;
        gpu_channel.rotation_key_value_y[i] = std::get<1>(rotation_keys[i]).y;
        gpu_channel.rotation_key_value_z[i] = std::get<1>(rotation_keys[i]).z;
        gpu_channel.rotation_key_value_w[i] = std::get<1>(rotation_keys[i]).w;
    }

    gpu_channel.scaling_key_count = static_cast<uint32_t>(scaling_keys.size());
    for (size_t i = 0; i < scaling_keys.size(); ++i) {
        gpu_channel.scaling_key_times[i] = static_cast<float>(std::get<0>(scaling_keys[i]));
        gpu_channel.scaling_key_value_x[i] = std::get<1>(scaling_keys[i]).x;
        gpu_channel.scaling_key_value_y[i] = std::get<1>(scaling_keys[i]).y;
        gpu_channel.scaling_key_value_z[i] = std::get<1>(scaling_keys[i]).z;
    }

    // Upload the channel data to the GPU buffer at the correct offset
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_channels_buffer));
    CHECK_GL_ERROR(glBufferSubData(
        GL_SHADER_STORAGE_BUFFER,
        static_cast<GLsizeiptr>(m_channels_count * sizeof(AnimationGPUChannel)),
        sizeof(AnimationGPUChannel),
        &gpu_channel
    ));
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
    m_channels_count += 1;

    return true;
}

Animation* Animation::CreateAnimation(
    double duration,
    double ticks_per_second,
    std::shared_ptr<Armature> armature
) noexcept {
    GLuint channels_buffer = 0;

    // Create a shader storage buffer (SSBO) to hold the animation channels data.
    CHECK_GL_ERROR(glGenBuffers(1, &channels_buffer));
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, channels_buffer));
    CHECK_GL_ERROR(glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        static_cast<GLsizeiptr>(MAX_ANIMATION_CHANNELS * sizeof(AnimationGPUChannel)),
        nullptr,
        GL_STATIC_DRAW
    ));
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

    return new Animation(
        duration,
        ticks_per_second,
        armature,
        channels_buffer,
        0u
    );
}
