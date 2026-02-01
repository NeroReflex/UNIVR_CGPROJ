#include "OpenGL.hpp"

#include "Scene.hpp"
#include "Camera/SpectatorCamera.hpp"
#include "Camera/OrthoSpectatorCamera.hpp"

#include "Shader.hpp"
#include "Program.hpp"
#include "Pipelines/ShadowedPipeline.hpp"
#include "Pipeline.hpp"

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
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(in_aPos.x, in_aPos.y, in_aPos.z, 1.0);\n"
    "}\0";

const char *fragmentShaderSource1 =
    "#version 320 es\n"
    "\n"
    "precision highp float;\n"
    "\n"
    //"layout(location = 0) in vec3 aPos;\n"
    "layout(location = 0) out vec4 color;\n"
    "void main()\n"
    "{\n"
    "   color = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\0";

const char *fragmentShaderSource2 =
    "#version 320 es\n"
    "\n"
    "precision highp float;\n"
    "\n"
    //"layout(location = 0) in vec3 aPos;\n"
    "layout(location = 0) out vec4 color;\n"
    "void main()\n"
    "{\n"
    "   color = vec4(1.0, 1.0, 0.0, 1.0);\n"
    "}\0";

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

    auto fragment1 = std::unique_ptr<FragmentShader>(FragmentShader::CompileShader(fragmentShaderSource1));
    assert(fragment1 != nullptr);

    auto fragment2 = std::unique_ptr<FragmentShader>(FragmentShader::CompileShader(fragmentShaderSource2));
    assert(fragment2 != nullptr);

    auto shaderProgram1 = std::unique_ptr<Program>(Program::LinkProgram(vertex.get(), nullptr, fragment1.get()));
    assert(shaderProgram1 != nullptr);

    auto shaderProgram2 = std::unique_ptr<Program>(Program::LinkProgram(vertex.get(), nullptr, fragment2.get()));
    assert(shaderProgram2 != nullptr);

    fragment2.reset();
    fragment1.reset();
    vertex.reset();

    GLuint VAO1, VBO1;
    GLuint num_indices_1 = 0;
    {
        CHECK_GL_ERROR(glGenVertexArrays(1, &VAO1));
        CHECK_GL_ERROR(glGenBuffers(1, &VBO1));

        auto vertices = std::vector<float>({
            -0.9f, -0.5f, 0.0f,
            0.1f, -0.5f, 0.0f,
            -0.4f,  0.5f, 0.0f,
        });

        num_indices_1 = static_cast<GLuint>(vertices.size() / 3);

        CHECK_GL_ERROR(glBindVertexArray(VAO1));

        CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, VBO1));
        CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));

        CHECK_GL_ERROR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0));
        CHECK_GL_ERROR(glEnableVertexAttribArray(0));
    }

    GLuint VAO2, VBO2;
    GLuint num_indices_2 = 0;
    {
        CHECK_GL_ERROR(glGenVertexArrays(1, &VAO2));
        CHECK_GL_ERROR(glGenBuffers(1, &VBO2));

        auto vertices = std::vector<float>({
            0.2f, -0.5f, 0.0f,
            0.9f, -0.5f, 0.0f,
            0.5f,  0.5f, 0.0f
        });

        num_indices_2 = static_cast<GLuint>(vertices.size() / 3);

        CHECK_GL_ERROR(glBindVertexArray(VAO2));

        CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, VBO2));
        CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));

        CHECK_GL_ERROR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0));
        CHECK_GL_ERROR(glEnableVertexAttribArray(0));
    }

    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(window))
    {
        process_input(window);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        const float ratio = width / (float) height;
 
        CHECK_GL_ERROR(glViewport(0, 0, width, height));
        CHECK_GL_ERROR(glClearColor(0.2f, 0.3f, 0.3f, 1.0f));
        CHECK_GL_ERROR(glClear(GL_COLOR_BUFFER_BIT));

        shaderProgram1->bind();
        CHECK_GL_ERROR(glBindVertexArray(VAO1));
        CHECK_GL_ERROR(glDrawArrays(GL_TRIANGLES, 0, num_indices_1));

        shaderProgram2->bind();
        CHECK_GL_ERROR(glBindVertexArray(VAO2));
        CHECK_GL_ERROR(glDrawArrays(GL_TRIANGLES, 0, num_indices_2));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    shaderProgram2.reset();
    shaderProgram1.reset();

    glfwDestroyWindow(window);

    glfwTerminate();

    return err;
}
