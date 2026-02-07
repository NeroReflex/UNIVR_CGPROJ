#pragma once

#include "../Pipeline.hpp"
#include "../Buffer.hpp"
#include "../Framebuffer.hpp"
#include "../RenderQuad.hpp"

#include <memory>
#include <array>
#include <vector>

#include <glm/glm.hpp>

class ShadowedPipeline : public Pipeline {
public:
    ~ShadowedPipeline() noexcept override;

    void render(const Scene *const scene) noexcept override;

    bool resize(GLsizei width, GLsizei height) noexcept override;

    static ShadowedPipeline* Create(GLsizei width, GLsizei height) noexcept;

protected:
    ShadowedPipeline(
        std::unique_ptr<Framebuffer>&& gbuffer,
        std::unique_ptr<Framebuffer>&& ssaobuffer,
        std::unique_ptr<Framebuffer>&& ssao_blur_buffer,
        std::array<std::unique_ptr<Framebuffer>, 2>&& lightbuffer,
        std::unique_ptr<Framebuffer>&& shadowbuffer,
        std::unique_ptr<Buffer>&& ssao_sample_buffer,
        std::unique_ptr<Texture>&& ssao_noise_texture,
        std::shared_ptr<Program> unshadowed_program,
        std::shared_ptr<Program> ssao_program,
        std::shared_ptr<Program> ssao_blur_program,
        std::shared_ptr<Program> ambient_lighting_program,
        std::shared_ptr<Program> depth_only_program,
        std::shared_ptr<Program> directional_lighting_program,
        std::shared_ptr<Program> cone_lighting_program,
        std::shared_ptr<Program> tone_mapping_program,
        std::shared_ptr<Program> post_program,
        std::shared_ptr<RenderQuad> m_render_quad,
        GLsizei width,
        GLsizei height
    ) noexcept;

    static std::vector<glm::vec4> GenerateSSAOSampleKernel(size_t sample_count) noexcept;
 
private:
    // G-buffer and intermediate framebuffers (owned)
    std::unique_ptr<Framebuffer> m_gbuffer;

    // SSAO framebuffer
    std::unique_ptr<Framebuffer> m_ssaobuffer;

    // SSAO framebuffer after blur
    std::unique_ptr<Framebuffer> m_ssao_blur_buffer;

    // lightbuffers: these get used alternatively as targets when applying
    // lights: this way one is the source and the other is the destination
    // where the destination is the source + new light contribution
    std::array<std::unique_ptr<Framebuffer>, 2> m_lightbuffer;

    // shadow map framebuffer: this holds the depth map(s) for shadowed lights
    std::unique_ptr<Framebuffer> m_shadowbuffer;

    // SSAO random samples
    std::unique_ptr<Buffer> m_ssao_sample_buffer;

    // SSAO noise texture
    std::unique_ptr<Texture> m_ssao_noise_texture;

    // G-buffer program to generate basic information
    std::shared_ptr<Program> m_mesh_program;

    // SSAO program
    std::shared_ptr<Program> m_ssao_program;

    // SSAO program
    std::shared_ptr<Program> m_ssao_blur_program;

    // Ambient lighting program to generate basic ambient lighting
    std::shared_ptr<Program> m_ambient_lighting_program;

    // used in shadow map(s) generation
    std::shared_ptr<Program> m_depth_only_program;
    
    // directional lighting program
    std::shared_ptr<Program> m_directional_lighting_program;
    
    // cone lighting program
    std::shared_ptr<Program> m_cone_lighting_program;

    // tone mapping program
    std::shared_ptr<Program> m_tone_mapping_program;

    // post-process program
    std::shared_ptr<Program> m_post_program;

    // Fullscreen quad (reusable)
    std::shared_ptr<RenderQuad> m_render_quad;

    // SSAO sample kernel
    std::vector<glm::vec4> m_ssao_samples;
};
