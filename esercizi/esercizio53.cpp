#include "OpenGL.hpp"

#include "Scene.hpp"
#include "Camera/SpectatorCamera.hpp"
#include "Camera/OrthoSpectatorCamera.hpp"

#include "Shader.hpp"
#include "Program.hpp"
#include "Pipelines/ShadowedPipeline.hpp"
#include "Pipeline.hpp"

#define STB_IMAGE_IMPLEMENTATION
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

const char *vertexShaderSourceLightsource =
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

const char *fragmentShaderSourceLightsource =
    "#version 320 es\n"
    "\n"
    "precision highp float;\n"
    "\n"
    "layout(location = 0) out vec4 color;\n"
    "\n"
    "uniform vec3 lightColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   color = vec4(lightColor, 1.0);\n"
    "}\0";

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
    "struct SpotLight {\n"
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
    "struct DirLight {\n"
    "    vec3 direction;\n"
    "\n"
    "    vec3 ambient;\n"
    "    vec3 diffuse;\n"
    "    vec3 specular;\n"
    "};\n"
    "\n"
    "struct PointLight {\n"
    "    vec3 position;\n"
    "\n"
    "    float constant;\n"
    "    float linear;\n"
    "    float quadratic;\n"
    "\n"
    "    vec3 ambient;\n"
    "    vec3 diffuse;\n"
    "    vec3 specular;\n"
    "};\n"
    "\n"
    "uniform DirLight directionalLight;\n"
    "\n"
    "uniform vec3 ambientlight;\n"
    "\n"
    "uniform SpotLight spotLights[6];\n"
    "uniform uint spotlights_count;\n"
    "\n"
    "uniform PointLight pointLights[6];\n"
    "uniform uint pointlights_count;\n"
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
    "vec3 calculate_spotlight(in SpotLight light, vec3 diffuseTerm, vec3 specularTerm, vec3 position) {\n"
    "   vec3 lightDir = normalize(light.position - position);\n"
    "\n"
    "   float theta = dot(lightDir, normalize(-light.direction));\n"
    "   float epsilon = light.cutOff - light.outerCutOff;\n"
    "   float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);\n"
    "\n"
    "   vec3 diffuseContrib = light.diffuse * max(dot(normalize(in_aNormal), lightDir), 0.0);\n"
    "\n"
    "   vec3 reflectDir = reflect(-lightDir, in_aNormal);\n"
    "   vec3 viewDir = normalize(viewPos - position);\n"
    "   vec3 specularContrib = light.specular * pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
    "\n"
    "   float distance    = length(light.position - position);\n"
    "   float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n"
    "\n"
    "   diffuseContrib *= attenuation * intensity;\n"
    "   specularContrib *= attenuation * intensity;\n"
    "   vec3 ambientContrib = light.ambient;\n"
    "\n"
    "   return ambientContrib * diffuseTerm + diffuseContrib * diffuseTerm + specularContrib * specularTerm;\n"
    "}\n"
    "vec3 calculate_ambient(vec3 ambientContrib, vec3 diffuseTerm) {\n"
    "   return ambientContrib * diffuseTerm;\n"
    "}\n"
    "\n"
    "vec3 calculate_directional_light(in DirLight light, vec3 diffuseTerm, vec3 specularTerm, vec3 normal, vec3 viewDir) {\n"
    "    vec3 lightDir = normalize(-light.direction);\n"
    "\n"
    "    float diff = max(dot(normal, lightDir), 0.0);\n"
    "\n"
    "    vec3 reflectDir = reflect(-lightDir, normal);\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
    "\n"
    "    vec3 ambientContrib = light.ambient;\n"
    "    vec3 diffuseContrib  = light.diffuse  * diff;\n"
    "    vec3 specularContrib = light.specular * spec;\n"
    "    return (ambientContrib * diffuseTerm + diffuseContrib * diffuseTerm + specularContrib * specularTerm);\n"
    "}\n"
    "\n"
    "vec3 calculate_point_light(in PointLight light, vec3 diffuseTerm, vec3 specularTerm, vec3 normal, vec3 position, vec3 viewDir) {\n"
    "    vec3 lightDir = normalize(light.position - position);\n"
    "\n"
    "    float diff = max(dot(normalize(normal), lightDir), 0.0);\n"
    "\n"
    "    vec3 reflectDir = reflect(-lightDir, normal);\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
    "\n"
    "    float distance = length(light.position - position);\n"
    "    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n"
    "\n"
    "    vec3 ambientContrib = light.ambient * attenuation;\n"
    "    vec3 diffuseContrib = light.diffuse * diff * attenuation;\n"
    "    vec3 specularContrib = light.specular * spec * attenuation;\n"
    "    return (ambientContrib * diffuseTerm + diffuseContrib * diffuseTerm + specularContrib * specularTerm);\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "   vec3 diffuseTerm = texture(material.diffuseTex, in_aTexCoord).rgb;\n"
    "   vec3 specularTerm = texture(material.specularTex, in_aTexCoord).rgb;\n"
    "   vec3 viewDir = normalize(viewPos - in_aPos);\n"
    "\n"
    "   vec3 ambientContrib = vec3(ambientlight);\n"
    "\n"
    "   vec3 result = calculate_ambient(ambientContrib, diffuseTerm);\n"
    "\n"
    "   vec3 normal = in_aNormal;\n"
    "\n"
    "   result += calculate_directional_light(directionalLight, diffuseTerm, specularTerm, normal, viewDir);\n"
    "\n"
    "   for (uint i = 0u; i < spotlights_count; ++i) {\n"
    "      result += calculate_spotlight(spotLights[i], diffuseTerm, specularTerm, in_aPos);\n"
    "   }\n"
    "\n"
    "   for (uint i = 0u; i < pointlights_count; ++i) {\n"
    "      result += calculate_point_light(pointLights[i], diffuseTerm, specularTerm, normal, in_aPos, viewDir);\n"
    "   }\n"
    "\n"
    "   color = vec4(result, 1.0);\n"
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

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    glm::float32 cutOff;
    glm::float32 outerCutOff;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    glm::float32 constant;
    glm::float32 linear;
    glm::float32 quadratic;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLight {
    glm::vec3 position;

    glm::float32 constant;
    glm::float32 linear;
    glm::float32 quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

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

    auto lightsource_vertex = std::unique_ptr<VertexShader>(VertexShader::CompileShader(vertexShaderSourceLightsource));
    assert(lightsource_vertex != nullptr);

    auto lightsource_fragment = std::unique_ptr<FragmentShader>(FragmentShader::CompileShader(fragmentShaderSourceLightsource));
    assert(lightsource_fragment != nullptr);

    auto lightsource_shaderProgram = std::unique_ptr<Program>(Program::LinkProgram(lightsource_vertex.get(), nullptr, lightsource_fragment.get()));
    assert(lightsource_shaderProgram != nullptr);

    lightsource_fragment.reset();
    lightsource_vertex.reset();
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

    std::vector<SpotLight> spotLights = {
        {
            .position = get_camera_position(),
            .direction = get_camera_direction(),
            .cutOff = glm::cos(glm::radians(12.5f)),
            .outerCutOff = glm::cos(glm::radians(17.5f)),

            .ambient = glm::vec3(0.2f, 0.2f, 0.2f),
            .diffuse = glm::vec3(0.5f, 0.5f, 0.5f),
            .specular = glm::vec3(1.0f, 1.0f, 1.0f),

            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f,
        }
    };

    DirLight dirLight = {
        .direction = glm::vec3(-0.2f, -1.0f, -0.3f),

        .ambient = glm::vec3(0.3f, 0.24f, 0.14f),
        .diffuse = glm::vec3(0.7f, 0.42f, 0.26f),
        .specular = glm::vec3(0.5f, 0.5f, 0.5f),
    };

    std::vector<PointLight> pointLights = {
        {
            .position = glm::vec3( 0.7f,  0.2f,  2.0f),

            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f,

            .ambient = glm::vec3(1.0f, 0.6f, 0.0f) * 0.1f,
            .diffuse = glm::vec3(1.0f, 0.6f, 0.0f),
            .specular = glm::vec3(1.0f, 0.6f, 0.0f),
        },
        {
            .position = glm::vec3( 2.3f, -3.3f, -4.0f),

            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f,

            .ambient = glm::vec3(1.0f, 0.0f, 0.0f) * 0.1f,
            .diffuse = glm::vec3(1.0f, 0.0f, 0.0f),
            .specular = glm::vec3(1.0f, 0.0f, 0.0f),
        },
        {
            .position = glm::vec3(-4.0f,  2.0f, -12.0f),

            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f,

            .ambient = glm::vec3(1.0f, 1.0, 0.0) * 0.1f,
            .diffuse = glm::vec3(1.0f, 1.0, 0.0),
            .specular = glm::vec3(1.0f, 1.0, 0.0),
        },
        {
            .position = glm::vec3( 0.0f,  0.0f, -3.0f),

            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f,

            .ambient = glm::vec3(0.2f, 0.2f, 1.0f) * 0.1f,
            .diffuse = glm::vec3(0.2f, 0.2f, 1.0f),
            .specular = glm::vec3(0.2f, 0.2f, 1.0f),
        }
    };

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
        glClearColor(0.75f, 0.52f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 view = build_view_matrix(); 

        const glm::mat4 projection = glm::perspective(glm::radians(fov), static_cast<GLfloat>(width) / static_cast<GLfloat>(height), 0.1f, 100.0f);

        shaderProgram->bind();

        CHECK_GL_ERROR(glBindVertexArray(VAO));

        shaderProgram->uniformMat4x4("view", view);
        shaderProgram->uniformMat4x4("projection", projection);
        shaderProgram->uniformVec3("viewPos", camera_center);

        // luci direzionali
        spotLights[0].position = get_camera_position();
        spotLights[0].direction = get_camera_direction();

        const glm::uint32 spotlights_count = spotLights.size();
        shaderProgram->uniformUint("spotlights_count", spotlights_count);
        for (glm::uint32 i = 0; i < spotlights_count; ++i) {
            const auto& light = spotLights[i];
            const std::string base_name = "spotLights[" + std::to_string(i) + "]";
            shaderProgram->uniformVec3(base_name + ".position", light.position);
            shaderProgram->uniformVec3(base_name + ".direction", light.direction);
            shaderProgram->uniformFloat(base_name + ".cutOff", light.cutOff);
            shaderProgram->uniformFloat(base_name + ".outerCutOff", light.outerCutOff);
            shaderProgram->uniformVec3(base_name + ".ambient", light.ambient);
            shaderProgram->uniformVec3(base_name + ".diffuse", light.diffuse);
            shaderProgram->uniformVec3(base_name + ".specular", light.specular);
            shaderProgram->uniformFloat(base_name + ".constant", light.constant);
            shaderProgram->uniformFloat(base_name + ".linear", light.linear);
            shaderProgram->uniformFloat(base_name + ".quadratic", light.quadratic);
        }

        // luce direzionale
        shaderProgram->uniformVec3("directionalLight.direction", dirLight.direction);
        shaderProgram->uniformVec3("directionalLight.ambient", dirLight.ambient);
        shaderProgram->uniformVec3("directionalLight.diffuse", dirLight.diffuse);
        shaderProgram->uniformVec3("directionalLight.specular", dirLight.specular);

        // punti luce
        const glm::uint32 pointlights_count = pointLights.size();
        shaderProgram->uniformUint("pointlights_count", pointlights_count);
        for (glm::uint32 i = 0; i < pointlights_count; ++i) {
            const auto& light = pointLights[i];
            const std::string base_name = "pointLights[" + std::to_string(i) + "]";
            shaderProgram->uniformVec3(base_name + ".position", light.position);
            shaderProgram->uniformFloat(base_name + ".constant", light.constant);
            shaderProgram->uniformFloat(base_name + ".linear", light.linear);
            shaderProgram->uniformFloat(base_name + ".quadratic", light.quadratic);
            shaderProgram->uniformVec3(base_name + ".ambient", light.ambient);
            shaderProgram->uniformVec3(base_name + ".diffuse", light.diffuse);
            shaderProgram->uniformVec3(base_name + ".specular", light.specular);
        }

        // luce ambientale
        shaderProgram->uniformVec3("ambientlight", glm::vec3(0.0f, 0.0f, 0.0f));

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

        // I punti di luce come casse
        lightsource_shaderProgram->bind();
        for (glm::uint32 i = 0; i < pointlights_count; ++i) {
            const auto& light = pointLights[i];

            CHECK_GL_ERROR(glBindVertexArray(VAO));

            glm::mat4 lightModel = glm::mat4(1.0f);
            lightModel = glm::translate(lightModel, light.position);
            lightModel = glm::scale(lightModel, glm::vec3(0.2f));

            lightsource_shaderProgram->uniformMat4x4("model", lightModel);
            lightsource_shaderProgram->uniformMat4x4("view", view);
            lightsource_shaderProgram->uniformMat4x4("projection", projection);

            lightsource_shaderProgram->uniformVec3("lightColor", light.diffuse);

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
