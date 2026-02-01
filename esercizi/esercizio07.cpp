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

const char *fragmentShaderSource =
    "#version 320 es\n"
    "\n"
    "precision highp float;\n"
    "\n"
    "uniform vec4 ourColor;\n"
    "\n"
    "layout(location = 0) out vec4 color;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   color = ourColor;\n"
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

    auto fragment = std::unique_ptr<FragmentShader>(FragmentShader::CompileShader(fragmentShaderSource));
    assert(fragment != nullptr);

    auto shaderProgram = std::unique_ptr<Program>(Program::LinkProgram(vertex.get(), nullptr, fragment.get()));
    assert(shaderProgram != nullptr);

    fragment.reset();
    vertex.reset();

    GLuint VAO, VBO, EBO;
    GLuint num_indices = 0;
    {
        CHECK_GL_ERROR(glGenVertexArrays(1, &VAO));
        CHECK_GL_ERROR(glGenBuffers(1, &VBO));
        CHECK_GL_ERROR(glGenBuffers(1, &EBO));

        auto vertices = std::vector<float>({
            -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
            0.0f,  0.5f, 0.0f
        });

        auto indices = std::vector<GLuint>({
            0, 1, 2
        });

        num_indices = static_cast<GLuint>(indices.size());

        CHECK_GL_ERROR(glBindVertexArray(VAO));

        CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, VBO));
        CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));

        CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO));
        CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW));

        CHECK_GL_ERROR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0));
        CHECK_GL_ERROR(glEnableVertexAttribArray(0));  
    }

    glm::vec4 triangle_color(0.0f, 0.0f, 0.0f, 1.0f);

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

        shaderProgram->bind();

        float timeValue = glfwGetTime();
        float greenValue = sin(timeValue) / 2.0f + 0.5f;
        triangle_color.g = greenValue;

        shaderProgram->uniformVec4("ourColor", triangle_color);

        CHECK_GL_ERROR(glBindVertexArray(VAO));
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    shaderProgram.reset();

    glfwDestroyWindow(window);

    glfwTerminate();

    return err;
}
