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

const char *vertexShaderSource =
    "#version 320 es\n"
    "\n"
    "precision highp float;\n"
    "\n"
    "layout (location = 0) in vec3 in_aPos;\n"
    "layout (location = 1) in vec2 in_aTexCoord;\n"
    "\n"
    "layout (location = 0) out vec2 out_aTexCoord;\n"
    "\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   mat4 transform = projection * view * model;\n"
    "   gl_Position = transform * vec4(in_aPos.x, in_aPos.y, in_aPos.z, 1.0);\n"
    "   out_aTexCoord = in_aTexCoord;\n"
    "}\0";

const char *fragmentShaderSource =
    "#version 320 es\n"
    "\n"
    "precision highp float;\n"
    "\n"
    "layout(location = 0) in vec2 in_aTexCoord;\n"
    "\n"
    "layout(location = 0) out vec4 color;\n"
    "\n"
    "uniform sampler2D texture0;"
    "uniform sampler2D texture1;"
    "\n"
    "void main()\n"
    "{\n"
    "   vec4 tex1 = texture(texture0, in_aTexCoord);\n"
    "   vec4 tex2 = texture(texture1, in_aTexCoord);\n"
    "   color = mix(tex1, tex2, 0.2);\n"
    "}\0";

static const auto camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

static glm::vec3 camera_center = glm::vec3(0.0f, 0.0f, 3.0f);

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

    GLuint VAO, VBO, texture1, texture2;

    int width1, height1, nrChannels1;
    unsigned char *texture_data1 = stbi_load("images/container.jpg", &width1, &height1, &nrChannels1, 0);
    assert(texture_data1 != nullptr);
    assert(nrChannels1 == 3);

    stbi_set_flip_vertically_on_load(true);  
    int width2, height2, nrChannels2;
    unsigned char *texture_data2 = stbi_load("images/awesomeface.png", &width2, &height2, &nrChannels2, 0);
    assert(texture_data2 != nullptr);
    assert(nrChannels2 == 4);

    GLuint num_vertices = 0;
    {
        CHECK_GL_ERROR(glGenVertexArrays(1, &VAO));
        CHECK_GL_ERROR(glGenBuffers(1, &VBO));
        
        auto vertices = std::vector<float>({
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
        });

        num_vertices = static_cast<GLuint>(vertices.size() / 5);

        CHECK_GL_ERROR(glGenTextures(1, &texture1));
        CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, texture1));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_data1));
        CHECK_GL_ERROR(glGenerateMipmap(GL_TEXTURE_2D));

        CHECK_GL_ERROR(glGenTextures(1, &texture2));
        CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, texture2));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width2, height2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data2));
        CHECK_GL_ERROR(glGenerateMipmap(GL_TEXTURE_2D));

        CHECK_GL_ERROR(glBindVertexArray(VAO));

        CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, VBO));
        CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));

        CHECK_GL_ERROR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
        CHECK_GL_ERROR(glEnableVertexAttribArray(0));

        CHECK_GL_ERROR(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
        CHECK_GL_ERROR(glEnableVertexAttribArray(1));
    }

    stbi_image_free(texture_data2);
    stbi_image_free(texture_data1);

    auto cubePositions = std::vector<glm::vec3>({
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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

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
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram->bind();

        glm::mat4 view = build_view_matrix(); 

        glm::mat4 projection;
        projection = glm::perspective(glm::radians(fov), static_cast<GLfloat>(width) / static_cast<GLfloat>(height), 0.1f, 100.0f);

        CHECK_GL_ERROR(glBindVertexArray(VAO));

        CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE0));
        CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, texture1));
        const GLint tex1_loc = glGetUniformLocation(shaderProgram->getProgram(), "texture0");
        if (tex1_loc >= 0) CHECK_GL_ERROR(glUniform1i(tex1_loc, 0));

        CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE1));
        CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, texture2));
        const GLint tex2_loc = glGetUniformLocation(shaderProgram->getProgram(), "texture1");
        if (tex2_loc >= 0) CHECK_GL_ERROR(glUniform1i(tex2_loc, 1));

        shaderProgram->uniformMat4x4("view", view);
        shaderProgram->uniformMat4x4("projection", projection);

        for(unsigned int i = 0; i < 10; i++)
        {
            glm::float32 rot_2 = sin((float)glfwGetTime());
            if (i % 3 != 0) rot_2 = 0.0f;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            glm::float32 angle = 20.0f * i;
            if (i % 3 == 0)
                angle = static_cast<glm::float32>(glfwGetTime()) * 25.0f;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            shaderProgram->uniformMat4x4("model", model);

            CHECK_GL_ERROR(glDrawArrays(GL_TRIANGLES, 0, num_vertices));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    shaderProgram.reset();

    glfwDestroyWindow(window);

    glfwTerminate();

    return err;
}
