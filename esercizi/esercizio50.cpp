#include "OpenGL.hpp"

#include "Scene.hpp"
#include "Camera/SpectatorCamera.hpp"
#include "Camera/OrthoSpectatorCamera.hpp"

#include "Shader.hpp"
#include "Program.hpp"
#include "Pipelines/ShadowedPipeline.hpp"
#include "Pipeline.hpp"

// This is done in Texture.cpp
//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <sstream>
#include <iostream>
#include <memory>
#include <cassert>

// settings
static const unsigned int SCR_WIDTH = 800;
static const unsigned int SCR_HEIGHT = 600;

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void process_input(GLFWwindow *const window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

const char *vertexShaderSource =
    "#version 320 es\n"
    "\n"
    "precision highp float;\n"
    "\n"
    "layout (location = 0) in vec3 in_aPos;\n"
    "layout (location = 1) in vec3 in_aNormal;\n"
    "layout (location = 2) in vec2 in_aTexCoord;\n"
    "\n"
    "uniform mat3 normal_matrix;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "\n"
    "layout (location = 0) out vec3 out_aPos;\n"
    "layout (location = 1) out vec3 out_aNormal;\n"
    "layout (location = 2) out vec2 out_aTexCoord;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   mat4 transform = projection * view * model;\n"
    "   gl_Position = transform * vec4(in_aPos.x, in_aPos.y, in_aPos.z, 1.0);\n"
    "   vec4 worldspace_pos = model * vec4(in_aPos, 1.0);\n"
    "   worldspace_pos /= worldspace_pos.w;\n"
    "   out_aPos = worldspace_pos.xyz;\n"
    "   out_aNormal = normal_matrix * in_aNormal;\n"
    "   out_aTexCoord = in_aTexCoord;\n"
    "}\0";

const char *fragmentShaderSource =
    "#version 320 es\n"
    "\n"
    "precision highp float;\n"
    "\n"
    "layout (location = 0) in vec3 in_aPos;\n"
    "layout (location = 1) in vec3 in_aNormal;\n"
    "layout (location = 2) in vec2 in_aTexCoord;\n"
    "\n"
    "layout(location = 0) out vec4 color;\n"
    "\n"
    "struct Light {\n"
    "    vec3 position;\n"
    "    vec3 direction;\n"
    "    float cutOff;\n"
    "    float outerCutOff;\n"
    "\n"
    "    vec3 ambient;\n"
    "    vec3 diffuse;\n"
    "    vec3 specular;\n"
    "\n"
    "    float constant;\n"
    "    float linear;\n"
    "    float quadratic;\n"
    "};\n"
    "\n"
    "uniform Light light;\n"
    "\n"
    "uniform vec3 viewPos;\n"
    "\n"
    "struct Material {\n"
    "    sampler2D diffuseTex;\n"
    "    sampler2D specularTex;\n"
    "    vec3 specular;\n"
    "    float shininess;\n"
    "};\n"
    "\n"
    "uniform Material material;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   vec3 diffuseTerm = texture(material.diffuseTex, in_aTexCoord).rgb;\n"
    "   vec3 specularTerm = texture(material.specularTex, in_aTexCoord).rgb;\n"
    "\n"
    "   vec3 lightDir = normalize(light.position - in_aPos);\n"
    "\n"
    "   float theta = dot(lightDir, normalize(-light.direction));\n"
    "   float epsilon = light.cutOff - light.outerCutOff;\n"
    "   float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);\n"
    "\n"
    "   vec3 ambientContrib = vec3(light.ambient);\n"
    "\n"
    "   vec3 diffuseContrib = light.diffuse * max(dot(normalize(in_aNormal), lightDir), 0.0);\n"
    "\n"
    "   vec3 reflectDir = reflect(-lightDir, in_aNormal);\n"
    "   vec3 viewDir = normalize(viewPos - in_aPos);\n"
    "   vec3 specularContrib = light.specular * pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
    "\n"
    "   float distance    = length(light.position - in_aPos);\n"
    "   float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n"
    "\n"
    "   diffuseContrib *= attenuation * intensity;\n"
    "   specularContrib *= attenuation * intensity;\n"
    "\n"
    "   color = vec4(ambientContrib * diffuseTerm + diffuseContrib * diffuseTerm + specularContrib * specularTerm, 1.0);\n"
    "}\0";

static const auto camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

static glm::vec3 camera_center = glm::vec3(0.0f, 0.0f, 6.0f);

static glm::float32 yaw = -90.0f;
static glm::float32 pitch = 0.0f;

static glm::float32 mouse_sensitivity = 0.1f;
static glm::float32 camera_speed = 2.5f;

static glm::float32 fov = 45.0f;

static glm::mat4 view_matrix_build(
    const glm::vec3& center,
    const glm::vec3& dir,
    const glm::vec3& up
) {
    const auto minus_dir = glm::normalize(-1.0f * dir);
    const auto right = glm::normalize(glm::cross(up, minus_dir));
    const auto camera_up = glm::normalize(glm::cross(minus_dir, right));

    const auto rotation = glm::mat4(
        right.x, right.y, right.z, 0.0f,
        camera_up.x, camera_up.y, camera_up.z, 0.0f,
        minus_dir.x, minus_dir.y, minus_dir.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    const auto translation = glm::mat4(
        1.0f, 0.0f, 0.0f, 0.0,
        0.0f, 1.0f, 0.0f, 0.0,
        0.0f, 0.0f, 1.0f, 0.0,
        -1.0 * center.x, -1.0 * center.y, -1.0 * center.z, 1.0f
    );

    return glm::transpose(rotation) * translation;
}

static glm::vec3 get_camera_direction(void) {
    glm::vec3 camera_dir;
    camera_dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    camera_dir.y = sin(glm::radians(pitch));
    camera_dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    return glm::normalize(camera_dir);
}

static glm::vec3 get_camera_position(void) {
    return camera_center;
}

static glm::mat4 build_view_matrix(void) {
    glm::vec3 camera_dir = get_camera_direction();

    return view_matrix_build(camera_center, glm::normalize(camera_dir), camera_up);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 90.0f)
        fov = 90.0f; 
}

static void process_input(GLFWwindow *const window, glm::float32 deltaTime)
{
    const auto delta_mov = camera_speed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera_center += delta_mov * glm::normalize(get_camera_direction());
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera_center -= delta_mov * glm::normalize(get_camera_direction());
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera_center -= delta_mov * glm::normalize(glm::cross(get_camera_direction(), camera_up));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera_center += delta_mov * glm::normalize(glm::cross(get_camera_direction(), camera_up));
}

static std::optional<glm::float32> lastX;
static std::optional<glm::float32> lastY;
static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!lastX.has_value() || !lastY.has_value()) {
        lastX = static_cast<glm::float32>(xpos);
        lastY = static_cast<glm::float32>(ypos);
    }

    glm::float32 xoffset = static_cast<glm::float32>(xpos) - lastX.value();
    glm::float32 yoffset = lastY.value() - static_cast<glm::float32>(ypos); // reversed since y-coordinates go from bottom to top

    lastX = static_cast<glm::float32>(xpos);
    lastY = static_cast<glm::float32>(ypos);

    xoffset *= mouse_sensitivity;
    yoffset *= mouse_sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
}

int main()
{
    int err = EXIT_SUCCESS;

    assert(glfwInit());

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    GLFWwindow *const window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "esercizio", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLAD for GLES2 loader
    if (!gladLoadGLES2((GLADloadfunc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD for GLES2" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    auto vertex = std::unique_ptr<VertexShader>(VertexShader::CompileShader(vertexShaderSource));
    assert(vertex != nullptr);

    auto fragment = std::unique_ptr<FragmentShader>(FragmentShader::CompileShader(fragmentShaderSource));
    assert(fragment != nullptr);

    auto shaderProgram = std::unique_ptr<Program>(Program::LinkProgram(vertex.get(), nullptr, fragment.get()));
    assert(shaderProgram != nullptr);

    fragment.reset();
    vertex.reset();

    GLuint VAO, VBO, specularTexture;

    auto diffuseTextureObj = std::shared_ptr<Texture>();
    auto specularTextureObj = std::shared_ptr<Texture>();

    GLuint num_vertices = 0;

    stbi_set_flip_vertically_on_load(true);  
    int diffuse_width, diffuse_height, diffuse_nrChannels;
    unsigned char *diffuse_texture_data = stbi_load("images/container2.png", &diffuse_width, &diffuse_height, &diffuse_nrChannels, 0);
    assert(diffuse_texture_data != nullptr);
    assert(diffuse_nrChannels == 4);

    int specular_width, specular_height, specular_nrChannels;
    unsigned char *specular_texture_data = stbi_load("images/container2_specular.png", &specular_width, &specular_height, &specular_nrChannels, 0);
    assert(specular_texture_data != nullptr);
    assert(specular_nrChannels == 4);

    {
        CHECK_GL_ERROR(glGenVertexArrays(1, &VAO));
        CHECK_GL_ERROR(glGenBuffers(1, &VBO));
        
        auto vertices = std::vector<float>({
            // positions          // normals           // texture coords
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
        });

        num_vertices = static_cast<GLuint>(vertices.size() / 8);

        diffuseTextureObj = std::shared_ptr<Texture>(
            Texture::Create2DTexture(
                diffuse_width,
                diffuse_height,
                TextureFormat::TEXTURE_FORMAT_RGBA8,
                TextureDataType::TEXTURE_DATA_TYPE_UNSIGNED_BYTE,
                diffuse_texture_data,
                TextureWrapMode::TEXTURE_WRAP_MODE_REPEAT,
                TextureWrapMode::TEXTURE_WRAP_MODE_REPEAT,
                TextureFilterMode::TEXTURE_FILTER_MODE_LINEAR,
                TextureFilterMode::TEXTURE_FILTER_MODE_LINEAR
            )
        );

        specularTextureObj = std::shared_ptr<Texture>(
            Texture::Create2DTexture(
                specular_width,
                specular_height,
                TextureFormat::TEXTURE_FORMAT_RGBA8,
                TextureDataType::TEXTURE_DATA_TYPE_UNSIGNED_BYTE,
                specular_texture_data,
                TextureWrapMode::TEXTURE_WRAP_MODE_REPEAT,
                TextureWrapMode::TEXTURE_WRAP_MODE_REPEAT,
                TextureFilterMode::TEXTURE_FILTER_MODE_LINEAR,
                TextureFilterMode::TEXTURE_FILTER_MODE_LINEAR
            )
        );

        CHECK_GL_ERROR(glBindVertexArray(VAO));

        CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, VBO));
        CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));

        CHECK_GL_ERROR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0));
        CHECK_GL_ERROR(glEnableVertexAttribArray(0));

        CHECK_GL_ERROR(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))));
        CHECK_GL_ERROR(glEnableVertexAttribArray(1));

        CHECK_GL_ERROR(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)((3 + 3) * sizeof(float))));
        CHECK_GL_ERROR(glEnableVertexAttribArray(2));
    }

    assert(diffuseTextureObj != nullptr);
    assert(specularTextureObj != nullptr);

    stbi_image_free(specular_texture_data);
    stbi_image_free(diffuse_texture_data);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    const auto cubePositions = std::vector<glm::vec3>({
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f,  2.0f, -2.5f),
        glm::vec3( 1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    });

    glfwSwapInterval(1);
    glm::float32 deltaTime = 0.0f;
    glm::float32 lastFrame = static_cast<glm::float32>(glfwGetTime());
    while (!glfwWindowShouldClose(window))
    {
        glm::float32 currentFrame = static_cast<glm::float32>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        process_input(window, deltaTime);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        const float ratio = width / (float) height;
 
        glEnable(GL_DEPTH_TEST);

        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 view = build_view_matrix(); 

        const glm::mat4 projection = glm::perspective(glm::radians(fov), static_cast<GLfloat>(width) / static_cast<GLfloat>(height), 0.1f, 100.0f);

        shaderProgram->bind();

        CHECK_GL_ERROR(glBindVertexArray(VAO));

        shaderProgram->uniformMat4x4("view", view);
        shaderProgram->uniformMat4x4("projection", projection);
        shaderProgram->uniformVec3("viewPos", camera_center);

        shaderProgram->uniformVec3("light.position", get_camera_position());
        shaderProgram->uniformVec3("light.direction", get_camera_direction());
        shaderProgram->uniformFloat("light.cutOff", glm::cos(glm::radians(12.5f)));
        shaderProgram->uniformFloat("light.outerCutOff", glm::cos(glm::radians(17.5f)));

        shaderProgram->uniformVec3("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
        shaderProgram->uniformVec3("light.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
        shaderProgram->uniformVec3("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));
        shaderProgram->uniformFloat("light.constant", 1.0f);
        shaderProgram->uniformFloat("light.linear", 0.09f);
        shaderProgram->uniformFloat("light.quadratic", 0.032f);

        // proprieta' del materiale
        shaderProgram->texture("material.diffuseTex", 0, *diffuseTextureObj.get());
        shaderProgram->texture("material.specularTex", 1, *specularTextureObj.get());
        shaderProgram->uniformFloat("material.shininess", 32.0f);

        for(unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            shaderProgram->uniformMat4x4("model", model);

            const glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
            shaderProgram->uniformMat3x3("normal_matrix", normal_matrix);

            CHECK_GL_ERROR(glDrawArrays(GL_TRIANGLES, 0, num_vertices));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    specularTextureObj.reset();
    diffuseTextureObj.reset();
    shaderProgram.reset();

    glfwDestroyWindow(window);

    glfwTerminate();

    return err;
}
