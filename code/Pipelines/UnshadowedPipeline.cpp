#include "UnshadowedPipeline.hpp"

#include <iostream>
#include <functional>
#include <vector>
#include <cassert>
#include <string>

#include "embedded_shaders_symbols.h"

static std::string vertex_shader_source_str(reinterpret_cast<const char*>(unshadowed_vert_glsl), unshadowed_vert_glsl_len);
static const GLchar* vertex_shader_source = vertex_shader_source_str.c_str();

static std::string fragment_shader_source_str(reinterpret_cast<const char*>(unshadowed_frag_glsl), unshadowed_frag_glsl_len);
static const GLchar* fragment_shader_source = fragment_shader_source_str.c_str();

// Fullscreen quad vertex shader
static std::string quad_vertex_shader_str(reinterpret_cast<const char*>(quad_vert_glsl), quad_vert_glsl_len);
static const GLchar* quad_vertex_shader = quad_vertex_shader_str.c_str();

// Lighting fragment shader: samples gbuffer and applies directional lights
static std::string directional_lighting_fragment_shader_str(reinterpret_cast<const char*>(unshadowed_directional_lighting_frag_glsl), unshadowed_directional_lighting_frag_glsl_len);
static const GLchar* directional_lighting_fragment_shader = directional_lighting_fragment_shader_str.c_str();

// Ambient lighting fragment shader: samples gbuffer and applies ambient light
static std::string ambient_lighting_fragment_shader_str(reinterpret_cast<const char*>(ambient_lighting_frag_glsl), ambient_lighting_frag_glsl_len);
static const GLchar* ambient_lighting_fragment_shader = ambient_lighting_fragment_shader_str.c_str();

// Post (blit) fragment shader
static std::string post_fragment_shader_str(reinterpret_cast<const char*>(post_frag_glsl), post_frag_glsl_len);
static const GLchar* post_fragment_shader = post_fragment_shader_str.c_str();

UnshadowedPipeline::UnshadowedPipeline(
    std::unique_ptr<Framebuffer>&& gbuffer,
    std::array<std::unique_ptr<Framebuffer>, 2>&& lightbuffer,
    std::shared_ptr<Program> unshadowed_program,
    std::shared_ptr<Program> ambient_lighting_program,
    std::shared_ptr<Program> lighting_program,
    std::shared_ptr<Program> post_program,
    std::shared_ptr<RenderQuad> m_render_quad,
    GLsizei width,
    GLsizei height
) noexcept :
    Pipeline(width, height),
    m_gbuffer(std::move(gbuffer)),
    m_lightbuffer(std::move(lightbuffer)),
    m_unshadowed_program(unshadowed_program),
    m_ambient_lighting_program(ambient_lighting_program),
    m_lighting_program(lighting_program),
    m_post_program(post_program),
    m_render_quad(m_render_quad)
{

}

UnshadowedPipeline::~UnshadowedPipeline() noexcept {
    // `m_gbuffer` and `m_lightbuffer` are managed by unique_ptr and will be
    // destroyed automatically.
    // `m_render_quad` will clean up its own VAO/VBO in its destructor.
}

void UnshadowedPipeline::render(const Scene& scene) noexcept {
    // 1) Geometry pass -> fill G-buffer (albedo, specular, normal) + depth
    if (m_gbuffer) {
        withFramebuffer(m_gbuffer.get(), [&]() {
            withEnabledDepthTest([&]() {
                withFaceCulling([&]() {
                    // build matrices
                    auto camera = scene.getCamera();
                    if (!camera) {
                        std::cerr << "Scene::render: No camera set in the scene!" << std::endl;
                        return;
                    }

                    GLint viewport[4] = {0,0,0,0};
                    CHECK_GL_ERROR(glGetIntegerv(GL_VIEWPORT, viewport));
                    const GLint width = viewport[2];
                    const GLint height = viewport[3];

                    const glm::mat4 proj = camera->getProjectionMatrix(static_cast<glm::uint32>(width), static_cast<glm::uint32>(height));
                    const glm::mat4 view = camera->getViewMatrix();

                    m_unshadowed_program->bind();

                    // Ensure samplers are bound to the correct texture units
                    const GLint uniTex = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_DiffuseTex");
                    if (uniTex >= 0) CHECK_GL_ERROR(glUniform1i(uniTex, 0));
                    const GLint uniSpecTex = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_SpecularTex");
                    if (uniSpecTex >= 0) CHECK_GL_ERROR(glUniform1i(uniSpecTex, 1));
                    const GLint uniDispTex = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_DisplacementTex");
                    if (uniDispTex >= 0) CHECK_GL_ERROR(glUniform1i(uniDispTex, 2));

                    const auto diffuse_color_location = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_DiffuseColor");
                    const auto material_flags_location = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_material_flags");
                    const auto shininess_location = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_Shininess");

                    // Draw each mesh using its own model matrix
                    scene.foreachMesh([&](const Mesh& mesh) {
                        const glm::mat4 model_matrix = mesh.getModelMatrix();
                        const glm::mat4 mvp = proj * view * model_matrix;
                        const glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_matrix)));

                        m_unshadowed_program->uniformMat4x4("u_MVP", mvp);
                        m_unshadowed_program->uniformMat4x4("u_ModelMatrix", model_matrix);
                        m_unshadowed_program->uniformMat3x3("u_NormalMatrix", normal_matrix);

                        mesh.draw(diffuse_color_location, material_flags_location, shininess_location);
                    });
                });
            });
        });
    }

    size_t last_used_lightbuffer_index = 0;

    const auto ambient_light = scene.getAmbientLight();
    const auto ambient_light_intensity = ambient_light ? ambient_light->getColorWithIntensity() : glm::vec3(0.0f);

    // 2) Ambient light pass: copy ambient light to ambient lightbuffer
    withFramebuffer(m_lightbuffer[last_used_lightbuffer_index].get(), [&]() {
        withFaceCulling([&]() {
            // bind unshadowed program
            m_ambient_lighting_program->bind();

            // bind gbuffer diffuse texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_gbuffer->getColorAttachment(0));
            const GLint uGDiff = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_GDiffuse");
            if (uGDiff >= 0) CHECK_GL_ERROR(glUniform1i(uGDiff, 0));

            m_ambient_lighting_program->uniformVec3("u_Ambient", ambient_light_intensity);

            // draw fullscreen quad
            if (m_render_quad) m_render_quad->draw();
        });
    });

    // 3) Lighting pass: sample G-buffer and output lit color to lightbuffer
    scene.foreachDirectionalLight([&](const DirectionalLight& dir_light) {
        const auto lightbuffer_index = (last_used_lightbuffer_index + 1) % 2;

        withFramebuffer(m_lightbuffer[lightbuffer_index].get(), [&]() {
            withFaceCulling([&]() {
                // bind lighting program
                m_lighting_program->bind();

                // bind gbuffer textures (diffuse, specular, normal, position)
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_gbuffer->getColorAttachment(0));
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, m_gbuffer->getColorAttachment(1));
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, m_gbuffer->getColorAttachment(2));
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, m_gbuffer->getColorAttachment(3));
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, m_lightbuffer[last_used_lightbuffer_index]->getColorAttachment(0));

                const GLint uGDiff = glGetUniformLocation(m_lighting_program->getProgram(), "u_GDiffuse");
                if (uGDiff >= 0) CHECK_GL_ERROR(glUniform1i(uGDiff, 0));
                const GLint uGSpec = glGetUniformLocation(m_lighting_program->getProgram(), "u_GSpecular");
                if (uGSpec >= 0) CHECK_GL_ERROR(glUniform1i(uGSpec, 1));
                const GLint uGNorm = glGetUniformLocation(m_lighting_program->getProgram(), "u_GNormal");
                if (uGNorm >= 0) CHECK_GL_ERROR(glUniform1i(uGNorm, 2));
                const GLint uGPos = glGetUniformLocation(m_lighting_program->getProgram(), "u_GPosition");
                if (uGPos >= 0) CHECK_GL_ERROR(glUniform1i(uGPos, 3));

                const GLint uLIP = glGetUniformLocation(m_lighting_program->getProgram(), "u_LightpassInput");
                if (uLIP >= 0) CHECK_GL_ERROR(glUniform1i(uLIP, 4));

                // bind directional light uniforms
                {
                    m_lighting_program->uniformVec3("u_LightDir", dir_light.getDirection());
                    m_lighting_program->uniformVec3("u_LightColor", dir_light.getColor());
                }

                // draw fullscreen quad
                if (m_render_quad) m_render_quad->draw();
            });
        });

        last_used_lightbuffer_index = lightbuffer_index;
    });

    // 4) Final pass: blit/lightbuffer -> default framebuffer using fullscreen quad (renderquad)
    withFaceCulling([&]() {
        bindDefaultFramebuffer();
        // set viewport to window size
        GLint viewport[4] = {0,0,0,0};
        CHECK_GL_ERROR(glGetIntegerv(GL_VIEWPORT, viewport));
        CHECK_GL_ERROR(glViewport(0, 0, viewport[2], viewport[3]));

        auto lightbuffer_color_attachment = m_lightbuffer[last_used_lightbuffer_index]->getColorAttachment(0);
        if (m_post_program && m_render_quad) {
            // convenience draw: program/sampler/texture handled by RenderQuad helper
            m_render_quad->drawWithTexture(*m_post_program, "u_SrcTex", lightbuffer_color_attachment, 0);
        }
    });
}

bool UnshadowedPipeline::resize(GLsizei width, GLsizei height) noexcept {
    // create new gbuffer: albedo(RGBA8), spec(RGBA8), normal(RGB32F), position(RGBA32F) + depth
    auto resized_gbuffer = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            width,
            height,
            {
                FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA8,
                FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA8,
                FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA8,
                FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA32F,
                FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA32F,
                FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA32F,
                FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_R32F
            },
            true,
            false
        )
    );
    if (!resized_gbuffer) {
        std::cerr << "UnshadowedPipeline::resize: Failed to resize G-buffer framebuffer!" << std::endl;
        return false;
    }

    // lightbuffer(s): single RGBA32F target
    auto resized_lightbuffer_0 = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            width,
            height,
            {
                FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGB32F
            },
            false,
            false
        )
    );
    if (!resized_lightbuffer_0) {
        std::cerr << "UnshadowedPipeline::resize: Failed to resize lightbuffer 0 framebuffer!" << std::endl;
        return false;
    }

    auto resized_lightbuffer_1 = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            width,
            height,
            {
                FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGB32F
            },
            false,
            false
        )
    );
    if (!resized_lightbuffer_1) {
        std::cerr << "UnshadowedPipeline::resize: Failed to resize lightbuffer 1 framebuffer!" << std::endl;
        return false;
    }

    auto resized_lightbuffer = std::array<std::unique_ptr<Framebuffer>, 2>{
        std::move(resized_lightbuffer_0),
        std::move(resized_lightbuffer_1)
    };

    // recreate framebuffers at new size
    m_gbuffer.swap(resized_gbuffer);
    m_lightbuffer.swap(resized_lightbuffer);

    return Pipeline::resize(width, height);
}

UnshadowedPipeline* UnshadowedPipeline::Create(GLsizei width, GLsizei height) noexcept {
    const auto vertex_shader = std::unique_ptr<VertexShader>(VertexShader::CompileShader(vertex_shader_source));
    assert(vertex_shader != nullptr && "Failed to create vertex shader");

    const auto fragment_shader = std::unique_ptr<FragmentShader>(FragmentShader::CompileShader(fragment_shader_source));
    assert(fragment_shader != nullptr && "Failed to create fragment shader");

    const auto unshadowed_program = std::shared_ptr<Program>(
        Program::LinkProgram(vertex_shader.get(), nullptr, fragment_shader.get())
    );
    assert(unshadowed_program != nullptr && "Failed to create shader program");

    // Create lighting program
    const auto quad_vert = std::unique_ptr<VertexShader>(
        VertexShader::CompileShader(quad_vertex_shader)
    );
    assert(quad_vert != nullptr && "Failed to compile quad vertex shader");

    const auto lighting_frag = std::unique_ptr<FragmentShader>(
        FragmentShader::CompileShader(directional_lighting_fragment_shader)
    );
    assert(lighting_frag != nullptr && "Failed to compile lighting fragment shader");

    auto lighting_program = std::shared_ptr<Program>(
        Program::LinkProgram(quad_vert.get(), nullptr, lighting_frag.get())
    );
    assert(lighting_program != nullptr && "Failed to create lighting program");

    const auto ambient_lighting_frag = std::unique_ptr<FragmentShader>(
        FragmentShader::CompileShader(ambient_lighting_fragment_shader)
    );
    assert(ambient_lighting_frag != nullptr && "Failed to compile lighting fragment shader");

    const auto ambient_lighting_program = std::shared_ptr<Program>(
        Program::LinkProgram(quad_vert.get(), nullptr, ambient_lighting_frag.get())
    );
    assert(ambient_lighting_program != nullptr && "Failed to create ambient lighting program");

    // Create post (blit) program
    const auto post_frag = std::unique_ptr<FragmentShader>(
        FragmentShader::CompileShader(post_fragment_shader)
    );
    assert(post_frag != nullptr && "Failed to compile post fragment shader");
    std::shared_ptr<Program> post_program = std::shared_ptr<Program>(
        Program::LinkProgram(quad_vert.get(), nullptr, post_frag.get())
    );
    assert(post_program != nullptr && "Failed to create post program");

    // Build framebuffers
    auto gbuffer = std::unique_ptr<Framebuffer>(Framebuffer::CreateFramebuffer(
        width,
        height,
        {
            FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA8,
            FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA8,
            FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA8,
            FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA32F,
            FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA32F,
            FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA32F,
            FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_R32F
        },
        true,
        false
    ));
    assert(gbuffer != nullptr && "Failed to create gbuffer");

    auto lightbuffer_0 = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            width,
            height,
            {
                FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGB32F
            },
            false,
            false
        )
    );
    assert(lightbuffer_0 != nullptr && "Failed to create lightbuffer 0");

    auto lightbuffer_1 = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            width,
            height,
            {
                FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGB32F
            },
            false,
            false
        )
    );
    assert(lightbuffer_1 != nullptr && "Failed to create lightbuffer 0");

    auto lightbuffer = std::array<std::unique_ptr<Framebuffer>, 2>{
        std::move(lightbuffer_0),
        std::move(lightbuffer_1)
    };

    // Create reusable fullscreen quad
    auto render_quad = std::shared_ptr<RenderQuad>(RenderQuad::Create());

    // Create the pipeline instance and fill extra members
    UnshadowedPipeline *const pipeline = new UnshadowedPipeline(
        std::move(gbuffer),
        std::move(lightbuffer),
        unshadowed_program,
        ambient_lighting_program,
        lighting_program,
        post_program,
        render_quad,
        width,
        height
    );

    return pipeline;
}
