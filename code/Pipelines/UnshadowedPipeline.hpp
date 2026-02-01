#pragma once

#include "../Pipeline.hpp"
#include "../Framebuffer.hpp"

#include <memory>
#include "../RenderQuad.hpp"


class UnshadowedPipeline : public Pipeline {
public:
    ~UnshadowedPipeline() noexcept override;

    void render(const Scene& scene) noexcept override;

    bool resize(GLsizei width, GLsizei height) noexcept override;

    static UnshadowedPipeline* Create(GLsizei width, GLsizei height) noexcept;

protected:
    UnshadowedPipeline(
        std::unique_ptr<Framebuffer>&& gbuffer,
        std::array<std::unique_ptr<Framebuffer>, 2>&& lightbuffer,
        std::shared_ptr<Program> unshadowed_program,
        std::shared_ptr<Program> ambient_lighting_program,
        std::shared_ptr<Program> lighting_program,
        std::shared_ptr<Program> post_program,
        std::shared_ptr<RenderQuad> m_render_quad,
        GLsizei width,
        GLsizei height
    ) noexcept;

private:
    // G-buffer and intermediate framebuffers (owned)
    std::unique_ptr<Framebuffer> m_gbuffer;

    // lightbuffers: they used alternatively as targets when applying lights
    // this way one is the source and the other is the destination
    // where the destination is the source + new light contribution
    std::array<std::unique_ptr<Framebuffer>, 2> m_lightbuffer;

    // G-buffer program to generate basic information
    std::shared_ptr<Program> m_unshadowed_program;

    // Ambient lighting program to generate basic ambient lighting
    std::shared_ptr<Program> m_ambient_lighting_program;

    // Lighting and post-process programs
    std::shared_ptr<Program> m_lighting_program;
    std::shared_ptr<Program> m_post_program;

    // Fullscreen quad (reusable)
    std::shared_ptr<RenderQuad> m_render_quad;
};
