#include "ShadowedPipeline.hpp"

#include <iostream>
#include <functional>
#include <vector>
#include <cassert>
#include <string>
#include <random>

#include "Scene.hpp"

#include "embedded_shaders_symbols.h"

static GLint find_ssbo_binding(GLuint program, const char* block_name) {
    const GLuint index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, block_name);
    if (index == GL_INVALID_INDEX) return -1;
    GLenum props[1] = { GL_BUFFER_BINDING };
    GLint binding = -1;
    glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, index, 1, props, 1, nullptr, &binding);
    return binding;
}

static std::string vertex_shader_source_str(reinterpret_cast<const char*>(mesh_vert_glsl), mesh_vert_glsl_len);
static const GLchar *const vertex_shader_source = vertex_shader_source_str.c_str();

static std::string geometry_shader_source_str(reinterpret_cast<const char*>(mesh_geom_glsl), mesh_geom_glsl_len);
static const GLchar *const geometry_shader_source = geometry_shader_source_str.c_str();

static std::string fragment_shader_source_str(reinterpret_cast<const char*>(mesh_frag_glsl), mesh_frag_glsl_len);
static const GLchar *const fragment_shader_source = fragment_shader_source_str.c_str();

static std::string depth_only_vertex_shader_source_str(reinterpret_cast<const char*>(mesh_vert_glsl), mesh_vert_glsl_len);
static const GLchar *const depth_only_vertex_shader_source = depth_only_vertex_shader_source_str.c_str();

static std::string depth_only_fragment_shader_source_str(reinterpret_cast<const char*>(depth_only_frag_glsl), depth_only_frag_glsl_len);
static const GLchar *const depth_only_fragment_shader_source = depth_only_fragment_shader_source_str.c_str();

// Fullscreen quad vertex shader
static std::string quad_vertex_shader_str(reinterpret_cast<const char*>(quad_vert_glsl), quad_vert_glsl_len);
static const GLchar *const quad_vertex_shader = quad_vertex_shader_str.c_str();

// Directional lighting fragment shader: samples gbuffer and applies directional lights
static std::string directional_lighting_fragment_shader_str(reinterpret_cast<const char*>(directional_lighting_frag_glsl), directional_lighting_frag_glsl_len);
static const GLchar *const directional_lighting_fragment_shader = directional_lighting_fragment_shader_str.c_str();

// Cone fragment shader: samples gbuffer and applies cone lights
static std::string cone_lighting_fragment_shader_str(reinterpret_cast<const char*>(cone_lighting_frag_glsl), cone_lighting_frag_glsl_len);
static const GLchar *const cone_lighting_fragment_shader = cone_lighting_fragment_shader_str.c_str();

// SSAO fragment shader: samples gbuffer and construct the SSAO framebuffer
static std::string ssao_fragment_shader_str(reinterpret_cast<const char*>(ssao_frag_glsl), ssao_frag_glsl_len);
static const GLchar *const ssao_fragment_shader = ssao_fragment_shader_str.c_str();

// SSAO blur shader: samples the SSAO framebuffer and apply blur
static std::string ssao_blur_fragment_shader_str(reinterpret_cast<const char*>(ssao_blur_frag_glsl), ssao_blur_frag_glsl_len);
static const GLchar *const ssao_blur_fragment_shader = ssao_blur_fragment_shader_str.c_str();

// Ambient lighting fragment shader: samples gbuffer and applies ambient light
static std::string ambient_lighting_fragment_shader_str(reinterpret_cast<const char*>(ambient_lighting_frag_glsl), ambient_lighting_frag_glsl_len);
static const GLchar *const ambient_lighting_fragment_shader = ambient_lighting_fragment_shader_str.c_str();

// Tone Mapping fragment shader
static std::string tone_mapping_fragment_shader_str(reinterpret_cast<const char*>(tone_mapping_frag_glsl), tone_mapping_frag_glsl_len);
static const GLchar *const tone_mapping_fragment_shader = tone_mapping_fragment_shader_str.c_str();

// Post (blit) fragment shader
static std::string post_fragment_shader_str(reinterpret_cast<const char*>(post_frag_glsl), post_frag_glsl_len);
static const GLchar *const post_fragment_shader = post_fragment_shader_str.c_str();

static const auto gbuffer_attachments = std::vector<FramebufferColorFormat>{
    FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA8, // diffuse
    FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA8, // specular
    FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA8, // normal tangentspace
    FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA32F, // position (worldspace)
    FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA32F, // normal (worldspace)
    FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA32F, // tangent (worldspace)
    FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_R32F // shininess
};

static const auto ssao_attachments = std::vector<FramebufferColorFormat>{
    FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_R32F
};

static const auto lightbuffer_attachments = std::vector<FramebufferColorFormat>{
    FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA32F
};

static const GLsizei ssao_samples_count = 64;

ShadowedPipeline::ShadowedPipeline(
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
) noexcept :
    Pipeline(width, height),
    m_gbuffer(std::move(gbuffer)),
    m_ssaobuffer(std::move(ssaobuffer)),
    m_ssao_blur_buffer(std::move(ssao_blur_buffer)),
    m_lightbuffer(std::move(lightbuffer)),
    m_shadowbuffer(std::move(shadowbuffer)),
    m_ssao_sample_buffer(std::move(ssao_sample_buffer)),
    m_ssao_noise_texture(std::move(ssao_noise_texture)),
    m_unshadowed_program(unshadowed_program),
    m_ssao_program(ssao_program),
    m_ssao_blur_program(ssao_blur_program),
    m_ambient_lighting_program(ambient_lighting_program),
    m_depth_only_program(depth_only_program),
    m_directional_lighting_program(directional_lighting_program),
    m_cone_lighting_program(cone_lighting_program),
    m_tone_mapping_program(tone_mapping_program),
    m_post_program(post_program),
    m_render_quad(m_render_quad)
{

}

ShadowedPipeline::~ShadowedPipeline() noexcept {
    // `m_gbuffer` and `m_lightbuffer` are managed by unique_ptr and will be
    // destroyed automatically.
    // `m_render_quad` will clean up its own VAO/VBO in its destructor.
}

void ShadowedPipeline::render(const Scene *const scene) noexcept {
    const auto camera = scene->getCamera();

    const GLint width = m_gbuffer->getWidth();
    const GLint height = m_gbuffer->getHeight();

    const auto proj = camera->getProjectionMatrix(
        static_cast<glm::uint32>(width),
        static_cast<glm::uint32>(height)
    );
    const auto view = camera->getViewMatrix();

    // 1) Geometry pass -> fill G-buffer (albedo, specular, normal) + depth
    if (m_gbuffer) {
        withFramebuffer(m_gbuffer.get(), [&]() {
            withEnabledDepthTest([&]() {
                withFaceCulling([&]() {
                    // build matrices
                    if (!camera) {
                        std::cerr << "Scene::render: No camera set in the scene!" << std::endl;
                        return;
                    }

                    // Draw each mesh using its own model matrix
                    m_unshadowed_program->bind();
                    m_unshadowed_program->uniformMat4x4("u_CustomGLPositionMatrix", glm::mat4(1.0f));

                    const GLint uniTex = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_DiffuseTex");
                    if (uniTex >= 0) CHECK_GL_ERROR(glUniform1i(uniTex, 0));
                    const GLint uniSpecTex = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_SpecularTex");
                    if (uniSpecTex >= 0) CHECK_GL_ERROR(glUniform1i(uniSpecTex, 1));
                    const GLint uniDispTex = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_DisplacementTex");
                    if (uniDispTex >= 0) CHECK_GL_ERROR(glUniform1i(uniDispTex, 2));

                    const auto diffuse_color_location = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_DiffuseColor");
                    const auto specular_color_location = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_SpecularColor");
                    const auto material_flags_location = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_material_flags");
                    const auto shininess_location = glGetUniformLocation(m_unshadowed_program->getProgram(), "u_Shininess");

                    // Find SSBO binding for `SkeletonBuffer` (if present) in the currently bound program
                    auto find_ssbo_binding = [](GLuint program, const char* block_name) -> GLint {
                        const GLuint index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, block_name);
                        if (index == GL_INVALID_INDEX) return -1;
                        GLenum props[1] = { GL_BUFFER_BINDING };
                        GLint binding = -1;
                        glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, index, 1, props, 1, nullptr, &binding);
                        return binding;
                    };

                    const GLint skeleton_binding = find_ssbo_binding(m_unshadowed_program->getProgram(), "SkeletonBuffer");

                    scene->foreachMesh([&](const Mesh& mesh) {
                        const glm::mat4 model_matrix = mesh.getModelMatrix();
                        const glm::mat4 mvp = proj * view * model_matrix;
                        const glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_matrix)));

                        m_unshadowed_program->uniformMat4x4("u_MVP", mvp);
                        m_unshadowed_program->uniformMat4x4("u_ModelMatrix", model_matrix);
                        m_unshadowed_program->uniformMat3x3("u_NormalMatrix", normal_matrix);

                        mesh.draw(
                            diffuse_color_location,
                            specular_color_location,
                            material_flags_location,
                            shininess_location,
                            skeleton_binding
                        );
                    });
                });
            });
        });
    }

    size_t last_used_lightbuffer_index = 0;

    const auto ambient_light = scene->getAmbientLight();
    const auto ambient_light_intensity = ambient_light ? ambient_light->getColorWithIntensity() : glm::vec3(0.0f);

    // 2.1) SSAO pass: generate the SSAO texture
    withFramebuffer(m_ssaobuffer.get(), [&]() {
        withFaceCulling([&]() {
            // bind unshadowed program
            m_ssao_program->bind();

            // bind gbuffer position and world-space normal textures
            // gNormal is at color attachment 4 (world-space normal)
            //m_ssao_program->framebufferColorAttachment("u_GNormal", GL_TEXTURE0, *m_gbuffer, 4);
            m_ssao_program->framebufferColorAttachment("u_GPosition", GL_TEXTURE1, *m_gbuffer, 3);

            // bind SSAO noise texture
            if (m_ssao_noise_texture) {
                m_ssao_program->texture("u_NoiseTex", GL_TEXTURE2, *m_ssao_noise_texture);
            }

            m_ssao_program->uniformStorageBuffer("u_SSAOSamples", *m_ssao_sample_buffer);
            m_ssao_program->uniformUint("u_SSAOSampleCount", static_cast<glm::uint32>(ssao_samples_count));
            m_ssao_program->uniformMat4x4("u_ViewMatrix", view);
            m_ssao_program->uniformMat4x4("u_ProjectionMatrix", proj);
            m_ssao_program->uniformFloat("u_radius", 15.0f);
            m_ssao_program->uniformUint("u_noise_dimensions", m_ssao_noise_texture ? m_ssao_noise_texture->getWidth() : 0u);

            // draw fullscreen quad
            if (m_render_quad) m_render_quad->draw();
        });
    });

    // 2.2) SSAO blur pass: generate the SSAO blurred texture
    withFramebuffer(m_ssao_blur_buffer.get(), [&]() {
        withFaceCulling([&]() {
            // bind unshadowed program
            m_ssao_blur_program->bind();

            // bind gbuffer position and normal textures
            m_ssao_blur_program->framebufferColorAttachment("in_ssao", GL_TEXTURE0, *m_ssaobuffer, 0);

            m_ssao_blur_program->uniformUint("u_noise_dimensions", m_ssao_noise_texture ? m_ssao_noise_texture->getWidth() : 0u);

            // draw fullscreen quad
            if (m_render_quad) m_render_quad->draw();
        });
    });

    // 3) Ambient light pass: copy ambient light to ambient lightbuffer
    withFramebuffer(m_lightbuffer[last_used_lightbuffer_index].get(), [&]() {
        withFaceCulling([&]() {
            // bind unshadowed program
            m_ambient_lighting_program->bind();

            // bind gbuffer diffuse texture and SSAO texture
            m_ambient_lighting_program->framebufferColorAttachment("u_GDiffuse", GL_TEXTURE0, *m_gbuffer, 0);
            m_ambient_lighting_program->framebufferColorAttachment("u_SSAO", GL_TEXTURE1, *m_ssao_blur_buffer, 0);

            m_ambient_lighting_program->uniformVec3("u_Ambient", ambient_light_intensity);

            // draw fullscreen quad
            if (m_render_quad) m_render_quad->draw();
        });
    });

    // 4.a) Directional lights pass: sample G-buffer and output lit color to lightbuffer
    scene->foreachDirectionalLight([&](const DirectionalLight& dir_light) {

        // set up light view/proj matrices (use minus sign to position the light "backwards" along its direction)
        const auto camera_pos = camera->getCameraPosition();
        const auto light_distance = camera->getFarPlane();
        const glm::vec3 light_pos = camera_pos - (
            dir_light.getDirection() * light_distance
        );
        const glm::mat4 light_view = glm::lookAt(
            light_pos,
            camera_pos,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        const auto sqrt_3 = static_cast<glm::float32>(1.73205080757);
        const glm::float32 aspect = 1.0f;
        // const glm::float32 top = camera->getFarPlane();
        const glm::float32 top = camera->getFarPlane() / 2.0f;
        const glm::float32 bottom = -top;
        const glm::float32 right = top * aspect;
        const glm::float32 left = -right;

        const glm::mat4 light_proj = glm::ortho(
            left,
            right,
            bottom,
            top,
            camera->getNearPlane(),
            camera->getFarPlane() * sqrt_3
        );
        const glm::mat4 light_space_matrix = light_proj * light_view;

        withFramebuffer(m_shadowbuffer.get(), [&]() {
            withFaceCulling([&]() {
                withEnabledDepthTest([&]() {
                    m_depth_only_program->bind();
                    m_depth_only_program->uniformMat4x4("u_MVP", glm::mat4(1.0f));

                    // Depth-only pass program bound; try to find skeleton binding for depth program (likely -1)
                    const GLint depth_skeleton_binding = find_ssbo_binding(m_depth_only_program->getProgram(), "SkeletonBuffer");

                    scene->foreachMesh([&](const Mesh& mesh) {
                        const glm::mat4 model_matrix = mesh.getModelMatrix();
                        const glm::mat4 ls = light_space_matrix * model_matrix;
                        m_depth_only_program->uniformMat4x4("u_CustomGLPositionMatrix", ls);
                        mesh.draw(
                            0,
                            0,
                            0,
                            0,
                            depth_skeleton_binding
                        );
                    });
                });
            });
        });

        const auto lightbuffer_index = (last_used_lightbuffer_index + 1) % 2;

        withFramebuffer(m_lightbuffer[lightbuffer_index].get(), [&]() {
            withFaceCulling([&]() {
                // bind lighting program
                m_directional_lighting_program->bind();

                // bind gbuffer textures (diffuse, specular, normal, position)
                m_directional_lighting_program->framebufferColorAttachment("u_GDiffuse", GL_TEXTURE0, *m_gbuffer, 0);
                m_directional_lighting_program->framebufferColorAttachment("u_GSpecular", GL_TEXTURE1, *m_gbuffer, 1);
                m_directional_lighting_program->framebufferColorAttachment("u_GNormalTangentSpace", GL_TEXTURE2, *m_gbuffer, 2);
                m_directional_lighting_program->framebufferColorAttachment("u_GPosition", GL_TEXTURE3, *m_gbuffer, 3);
                m_directional_lighting_program->framebufferColorAttachment("u_GNormal", GL_TEXTURE4, *m_gbuffer, 4);
                m_directional_lighting_program->framebufferColorAttachment("u_GTangent", GL_TEXTURE5, *m_gbuffer, 5);
                m_directional_lighting_program->framebufferColorAttachment("u_LightpassInput", GL_TEXTURE6, *m_lightbuffer[last_used_lightbuffer_index], 0);
                m_directional_lighting_program->framebufferDepthAttachment("u_LDepthTexture", GL_TEXTURE7, *m_shadowbuffer);

                // bind directional light uniforms
                {
                    m_directional_lighting_program->uniformVec3("u_LightDir", dir_light.getDirection());
                    m_directional_lighting_program->uniformVec3("u_LightColor", dir_light.getColorWithIntensity());
                    m_directional_lighting_program->uniformMat4x4("u_LightSpaceMatrix", light_space_matrix);
                }

                // draw fullscreen quad
                if (m_render_quad) m_render_quad->draw();
            });
        });

        last_used_lightbuffer_index = lightbuffer_index;
    });

    // 4.b) Cone (spot) lights: unshadowed cone lights applied additively
    scene->foreachConeLight([&](const ConeLight& cone) {
        const auto lightbuffer_index = (last_used_lightbuffer_index + 1) % 2;

        const auto light_znear = cone.getZNear();
        const auto light_zfar = cone.getZFar();
        const auto light_angle = cone.getAngleRadians();
        const auto aspect = static_cast<glm::float32>(m_shadowbuffer->getWidth()) /
            static_cast<glm::float32>(m_shadowbuffer->getHeight());
        const glm::vec3 light_pos = cone.getPosition();
        const glm::vec3 light_dir = cone.getDirection();
        const glm::mat4 light_view = glm::lookAt(
            light_pos,
            light_pos + light_dir,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        const auto light_proj = glm::perspective(
            light_angle,
            aspect,
            light_znear,
            light_zfar
        );

        const glm::mat4 light_space_matrix = light_proj * light_view;

        // TODO: if this light has no chances of affecting any portion of visible scene, skip it

        withFramebuffer(m_shadowbuffer.get(), [&]() {
            withFaceCulling([&]() {
                withEnabledDepthTest([&]() {
                    m_depth_only_program->bind();
                    m_depth_only_program->uniformMat4x4("u_MVP", glm::mat4(1.0f));

                    // Depth-only pass for cone light
                    const GLint depth_skeleton_binding = find_ssbo_binding(m_depth_only_program->getProgram(), "SkeletonBuffer");

                    scene->foreachMesh([&](const Mesh& mesh) {
                        const glm::mat4 model_matrix = mesh.getModelMatrix();
                        const glm::mat4 ls = light_space_matrix * model_matrix;
                        m_depth_only_program->uniformMat4x4("u_CustomGLPositionMatrix", ls);
                        mesh.draw(
                            0,
                            0,
                            0,
                            0,
                            depth_skeleton_binding
                        );
                    });
                });
            });
        });

        withFramebuffer(m_lightbuffer[lightbuffer_index].get(), [&]() {
            withFaceCulling([&]() {
                // bind cone lighting program
                m_cone_lighting_program->bind();

                m_cone_lighting_program->framebufferColorAttachment("u_GDiffuse", GL_TEXTURE0, *m_gbuffer, 0);
                m_cone_lighting_program->framebufferColorAttachment("u_GSpecular", GL_TEXTURE1, *m_gbuffer, 1);
                m_cone_lighting_program->framebufferColorAttachment("u_GNormalTangentSpace", GL_TEXTURE2, *m_gbuffer, 2);
                m_cone_lighting_program->framebufferColorAttachment("u_GPosition", GL_TEXTURE3, *m_gbuffer, 3);
                m_cone_lighting_program->framebufferColorAttachment("u_GNormal", GL_TEXTURE4, *m_gbuffer, 4);
                m_cone_lighting_program->framebufferColorAttachment("u_GTangent", GL_TEXTURE5, *m_gbuffer, 5);
                m_cone_lighting_program->framebufferColorAttachment("u_LightpassInput", GL_TEXTURE6, *m_lightbuffer[last_used_lightbuffer_index], 0);
                m_cone_lighting_program->framebufferDepthAttachment("u_LDepthTexture", GL_TEXTURE7, *m_shadowbuffer);

                // bind cone light uniforms
                {
                    m_cone_lighting_program->uniformVec3("u_LightColor", cone.getColorWithIntensity());
                    m_cone_lighting_program->uniformVec3("u_LightPosition", cone.getPosition());
                    m_cone_lighting_program->uniformVec3("u_LightDirection", cone.getDirection());
                    m_cone_lighting_program->uniformMat4x4("u_LightSpaceMatrix", light_space_matrix);

                    // compute inner/outer cone cutoffs from the light angle and cone properties
                    const float outerHalf = static_cast<float>(light_angle * 0.5f);
                    const float innerHalf = outerHalf * cone.getInnerRatio();
                    const float cutOff = std::cos(innerHalf);
                    const float outerCutOff = std::cos(outerHalf);

                    m_cone_lighting_program->uniformFloat("u_LightCutOff", cutOff);
                    m_cone_lighting_program->uniformFloat("u_LightOuterCutOff", outerCutOff);

                    // distance attenuation constants from cone light
                    m_cone_lighting_program->uniformFloat("u_LightConstant", cone.getConstant());
                    m_cone_lighting_program->uniformFloat("u_LightLinear", cone.getLinear());
                    m_cone_lighting_program->uniformFloat("u_LightQuadratic", cone.getQuadratic());
                }

                // draw fullscreen quad
                if (m_render_quad) m_render_quad->draw();
            });
        });

        last_used_lightbuffer_index = lightbuffer_index;
    });

    // 5) Tone mapping pass: apply tone mapping to the last used lightbuffer
    {
        const auto lightbuffer_index = (last_used_lightbuffer_index + 1) % 2;
        withFramebuffer(m_lightbuffer[lightbuffer_index].get(), [&]() {
            withFaceCulling([&]() {
                m_tone_mapping_program->bind();

                auto lightbuffer_color_attachment = m_lightbuffer[last_used_lightbuffer_index]->getColorAttachment(0);

                m_render_quad->drawWithTexture(*m_tone_mapping_program, "u_SrcTex", lightbuffer_color_attachment, 0);
            });
        });
        last_used_lightbuffer_index = lightbuffer_index;
    }

    // 6) Final pass: blit/lightbuffer -> default framebuffer using fullscreen quad (renderquad)
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

bool ShadowedPipeline::resize(GLsizei width, GLsizei height) noexcept {
    // create new gbuffer: albedo(RGBA8), spec(RGBA8), normal(RGB32F), position(RGBA32F) + depth
    auto resized_gbuffer = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            width,
            height,
            gbuffer_attachments,
            true,
            false
        )
    );
    if (!resized_gbuffer) {
        std::cerr << "ShadowedPipeline::resize: Failed to resize G-buffer framebuffer!" << std::endl;
        return false;
    }

    auto resized_ssaobuffer = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            width,
            height,
            ssao_attachments,
            false,
            false
        )
    );
    if (!resized_ssaobuffer) {
        std::cerr << "ShadowedPipeline::resize: Failed to resize SSAO framebuffer!" << std::endl;
        return false;
    }

    // lightbuffer(s): single RGBA32F target
    auto resized_lightbuffer_0 = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            width,
            height,
            lightbuffer_attachments,
            false,
            false
        )
    );
    if (!resized_lightbuffer_0) {
        std::cerr << "ShadowedPipeline::resize: Failed to resize lightbuffer 0 framebuffer!" << std::endl;
        return false;
    }

    auto resized_lightbuffer_1 = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            width,
            height,
            lightbuffer_attachments,
            false,
            false
        )
    );
    if (!resized_lightbuffer_1) {
        std::cerr << "ShadowedPipeline::resize: Failed to resize lightbuffer 1 framebuffer!" << std::endl;
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

ShadowedPipeline* ShadowedPipeline::Create(GLsizei width, GLsizei height) noexcept {
    const auto vertex_shader = std::unique_ptr<VertexShader>(VertexShader::CompileShader(vertex_shader_source));
    assert(vertex_shader != nullptr && "Failed to create vertex shader");

    const auto geometry_shader = std::unique_ptr<GeometryShader>(GeometryShader::CompileShader(geometry_shader_source));
    assert(geometry_shader != nullptr && "Failed to create geometry shader");

    const auto fragment_shader = std::unique_ptr<FragmentShader>(FragmentShader::CompileShader(fragment_shader_source));
    assert(fragment_shader != nullptr && "Failed to create fragment shader");

    const auto unshadowed_program = std::shared_ptr<Program>(
        Program::LinkProgram(vertex_shader.get(), geometry_shader.get(), fragment_shader.get())
    );
    assert(unshadowed_program != nullptr && "Failed to create shader program");

    // quad rendering shader for lighting and post-processing
    const auto quad_vert = std::unique_ptr<VertexShader>(
        VertexShader::CompileShader(quad_vertex_shader)
    );
    assert(quad_vert != nullptr && "Failed to compile quad vertex shader");

    // directional lighting program
    const auto directional_lighting_frag = std::unique_ptr<FragmentShader>(
        FragmentShader::CompileShader(directional_lighting_fragment_shader)
    );
    assert(directional_lighting_frag != nullptr && "Failed to compile directional lighting fragment shader");

    auto directional_lighting_program = std::shared_ptr<Program>(
        Program::LinkProgram(quad_vert.get(), nullptr, directional_lighting_frag.get())
    );
    assert(directional_lighting_program != nullptr && "Failed to create directional lighting program");

    // SSAO program
    const auto ssao_frag = std::unique_ptr<FragmentShader>(
        FragmentShader::CompileShader(ssao_fragment_shader)
    );
    assert(ssao_frag != nullptr && "Failed to compile SSAO fragment shader");

    const auto ssao_program = std::shared_ptr<Program>(
        Program::LinkProgram(quad_vert.get(), nullptr, ssao_frag.get())
    );
    assert(ssao_program != nullptr && "Failed to create SSAO program");

    // SSAO blur program
    const auto ssao_blur_frag = std::unique_ptr<FragmentShader>(
        FragmentShader::CompileShader(ssao_blur_fragment_shader)
    );
    assert(ssao_blur_frag != nullptr && "Failed to compile SSAO blur fragment shader");

    const auto ssao_blur_program = std::shared_ptr<Program>(
        Program::LinkProgram(quad_vert.get(), nullptr, ssao_blur_frag.get())
    );
    assert(ssao_blur_program != nullptr && "Failed to create SSAO blur program");

    // ambient lighting program
    const auto ambient_lighting_frag = std::unique_ptr<FragmentShader>(
        FragmentShader::CompileShader(ambient_lighting_fragment_shader)
    );
    assert(ambient_lighting_frag != nullptr && "Failed to compile ambient lighting fragment shader");

    const auto ambient_lighting_program = std::shared_ptr<Program>(
        Program::LinkProgram(quad_vert.get(), nullptr, ambient_lighting_frag.get())
    );
    assert(ambient_lighting_program != nullptr && "Failed to create ambient lighting program");

    // cone lighting program
    const auto cone_lighting_frag = std::unique_ptr<FragmentShader>(
        FragmentShader::CompileShader(cone_lighting_fragment_shader)
    );
    assert(cone_lighting_frag != nullptr && "Failed to compile cone lighting fragment shader");

    const auto cone_lighting_program = std::shared_ptr<Program>(
        Program::LinkProgram(quad_vert.get(), nullptr, cone_lighting_frag.get())
    );
    assert(cone_lighting_program != nullptr && "Failed to create cone lighting program");

    // tone mapping lighting program
    const auto tone_mapping_frag = std::unique_ptr<FragmentShader>(
        FragmentShader::CompileShader(tone_mapping_fragment_shader)
    );
    assert(tone_mapping_frag != nullptr && "Failed to compile tone mapping fragment shader");

    const auto tone_mapping_program = std::shared_ptr<Program>(
        Program::LinkProgram(quad_vert.get(), nullptr, tone_mapping_frag.get())
    );
    assert(tone_mapping_program != nullptr && "Failed to create tone mapping program");

    // shadowmap program
    const auto depth_only_vert = std::unique_ptr<VertexShader>(
        VertexShader::CompileShader(depth_only_vertex_shader_source)
    );
    const auto depth_only_frag = std::unique_ptr<FragmentShader>(
        FragmentShader::CompileShader(depth_only_fragment_shader_source)
    );
    auto depth_only_program = std::shared_ptr<Program>(
        Program::LinkProgram(depth_only_vert.get(), nullptr, depth_only_frag.get())
    );

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
        gbuffer_attachments,
        true,
        false
    ));
    assert(gbuffer != nullptr && "Failed to create G-Buffer");

    // Build framebuffers
    auto ssaobuffer = std::unique_ptr<Framebuffer>(Framebuffer::CreateFramebuffer(
        width,
        height,
        ssao_attachments,
        false,
        false
    ));
    assert(ssaobuffer != nullptr && "Failed to create SSAO framebuffer");

    auto ssao_blur_buffer = std::unique_ptr<Framebuffer>(Framebuffer::CreateFramebuffer(
        width,
        height,
        ssao_attachments,
        false,
        false
    ));
    assert(ssao_blur_buffer != nullptr && "Failed to create SSAO blur framebuffer");

    auto lightbuffer_0 = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            width,
            height,
            lightbuffer_attachments,
            false,
            false
        )
    );
    assert(lightbuffer_0 != nullptr && "Failed to create lightbuffer 0");

    auto lightbuffer_1 = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            width,
            height,
            lightbuffer_attachments,
            false,
            false
        )
    );
    assert(lightbuffer_1 != nullptr && "Failed to create lightbuffer 0");

    auto lightbuffer = std::array<std::unique_ptr<Framebuffer>, 2>{
        std::move(lightbuffer_0),
        std::move(lightbuffer_1)
    };

    auto shadowbuffer = std::unique_ptr<Framebuffer>(
        Framebuffer::CreateFramebuffer(
            4096,
            4096,
            {},
            true,
            false
        )
    );
    assert(shadowbuffer != nullptr && "Failed to create shadowbuffer");

    const auto ssa_samples = ShadowedPipeline::GenerateSSAOSampleKernel(ssao_samples_count);
    auto ssao_sample_buffer = std::unique_ptr<Buffer>(
        Buffer::CreateBuffer(
            ssa_samples.data(),
            sizeof(glm::vec4) * ssao_samples_count
        )
    );
    assert(ssao_sample_buffer != nullptr && "Failed to create SSAO sample buffer");

    // Create reusable fullscreen quad
    auto render_quad = std::shared_ptr<RenderQuad>(RenderQuad::Create());


    std::vector<glm::uint32> ssao_noise_data;
    ssao_noise_data.reserve(16);
    std::uniform_int_distribution<glm::uint32> randomUInt(0, 0xFFFFFFFFu);
    std::default_random_engine generator;
    for (size_t i = 0; i < 16; ++i) {
        ssao_noise_data.push_back(randomUInt(generator));
    }

    auto ssao_noise_texture = std::unique_ptr<Texture>(Texture::Create2DTexture(
        4,
        4,
        TextureFormat::TEXTURE_FORMAT_R32UI,
        TextureDataType::TEXTURE_DATA_TYPE_UNSIGNED_INT,
        ssao_noise_data.data(),
        TextureWrapMode::TEXTURE_WRAP_MODE_REPEAT,
        TextureWrapMode::TEXTURE_WRAP_MODE_REPEAT,
        TextureFilterMode::TEXTURE_FILTER_MODE_NEAREST,
        TextureFilterMode::TEXTURE_FILTER_MODE_NEAREST
    ));

    assert(ssao_sample_buffer != nullptr && "Failed to create SSAO random rotation texture");

    // Create the pipeline instance and fill extra members
    ShadowedPipeline *const pipeline = new ShadowedPipeline(
        std::move(gbuffer),
        std::move(ssaobuffer),
        std::move(ssao_blur_buffer),
        std::move(lightbuffer),
        std::move(shadowbuffer),
        std::move(ssao_sample_buffer),
        std::move(ssao_noise_texture),
        unshadowed_program,
        ssao_program,
        ssao_blur_program,
        ambient_lighting_program,
        depth_only_program,
        directional_lighting_program,
        cone_lighting_program,
        tone_mapping_program,
        post_program,
        render_quad,
        width,
        height
    );

    return pipeline;
}

std::vector<glm::vec4> ShadowedPipeline::GenerateSSAOSampleKernel(size_t sample_count) noexcept {
    std::vector<glm::vec4> samples;
    samples.reserve(sample_count);

    std::uniform_real_distribution<glm::float32> randomFloats(0.0, 1.0);
    std::default_random_engine generator;

    for (size_t i = 0; i < sample_count; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        // scale samples so they're more aligned to the center of the kernel
        glm::float32 scale = static_cast<glm::float32>(i) / static_cast<glm::float32>(sample_count);
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;

        samples.emplace_back(sample, 0.0f);
    }

    return samples;
}