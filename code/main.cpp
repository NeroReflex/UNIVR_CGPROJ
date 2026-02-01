#include "OpenGL.hpp"

#include <SDL2/SDL.h>
#include <SDL_keycode.h>

// ImGui header files
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "Scene.hpp"
#include "Camera/SpectatorCamera.hpp"
#include "Camera/OrthoSpectatorCamera.hpp"

#include "Pipelines/ShadowedPipeline.hpp"
#include "Pipeline.hpp"

#include <sstream>
#include <iostream>
#include <memory>

// settings
static const unsigned int SCR_WIDTH = 800;
static const unsigned int SCR_HEIGHT = 600;

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_3d_asset>" << std::endl;
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Request an OpenGL ES 3.2 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    // Request a 24-bit depth buffer for proper depth testing
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("Project",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          SCR_WIDTH, SCR_HEIGHT,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        std::cerr << "Failed to create OpenGL ES context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    // Initialize GLAD for GLES2 loader
    if (!gladLoadGLES2((GLADloadfunc)SDL_GL_GetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD for GLES2" << std::endl;
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    // Use GLSL ES 3.0 shader prefix that ImGui backend recognizes
    ImGui_ImplOpenGL3_Init("#version 300 es");

    std::shared_ptr<Scene> scene = std::make_shared<Scene>();

    auto camera = std::shared_ptr<Camera>(
        new SpectatorCamera(
            glm::vec3(152.0f, 650.0f, -8.5f),
            10.0f,
            5000.0f,
            Camera::HeadUp,
            65.0f,
            glm::radians(-17.249994f),
            glm::radians(-0.029999956f)
        )
        /*
        new OrthoSpectatorCamera(
            glm::vec3(152.0, 650.0, -8.5),
            1.0,
            10000.0,
            Camera::HeadUp,
            650.0f, // ortho half-height to approximate previous framing
            glm::radians(-17.249994f),
            glm::radians(-0.029999956f)
        )
        */
    );

    scene->load_asset(argv[1]);

    scene->setAmbientLight(
        AmbientLight(
            glm::vec3(0.2f, 0.2f, 0.2f),
            1.0f
        )
    );

    scene->setDirectionalLight("Sun", DirectionalLight(
        glm::vec3(0.5f, 0.5f, 0.5f),
        10.0f,
        glm::vec3(-0.25f, -1.0f, 0.0f)
    ));

    // Add a default cone/spot light (angle stored in radians)
    const auto default_spot_light = ConeLight(
        glm::vec3(1.0f, 0.9f, 0.8f),
        1.0f,
        glm::vec3(152.0f, 650.0f, -8.5f),
        glm::vec3(0.5f, 0.0f, 5.0f),
        glm::radians(16.0f),
        10.0f,
        5000.0f
    );
    scene->setConeLight("Spot", default_spot_light);

    // La pila del giocatore
    const auto player_spot = ConeLight(
        default_spot_light.getColor(),
        default_spot_light.getIntensity(),
        camera->getCameraPosition(),
        camera->getOrientation(),
        default_spot_light.getAngleRadians(),
        default_spot_light.getZNear(),
        default_spot_light.getZFar()
    );
    scene->setConeLight("PlayerSpot", player_spot);

    // Store default light instances for re-adding when toggled
    DirectionalLight sun_default = *scene->getDirectionalLight("Sun");
    ConeLight spot_default = *scene->getConeLight("Spot");
    const auto player_spot_default = *scene->getConeLight("PlayerSpot");
    AmbientLight ambient_default = *scene->getAmbientLight();

    scene->setCamera(camera);

    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    auto pipeline = std::shared_ptr<Pipeline>(ShadowedPipeline::Create(width, height));
    if (!pipeline) {
        std::cerr << "Failed to create graphic pipeline" << std::endl;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    const float moveUnitPerSecond = 350.0f;
    const float mouseSensitivity = 0.015f;
    bool camera_locked = false;
    bool sun_enabled = true;
    bool spot_enabled = true;
    bool player_spot_enabled = true;
    bool ambient_enabled = true;

    bool running = true;
    uint32_t lastTicks = SDL_GetTicks();
    float fps_display = 0.0f;
    float ms_display = 0.0f;
    int fps_frames = 0;
    float fps_accumulator = 0.0f;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                running = false;
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
                glViewport(0, 0, event.window.data1, event.window.data2);
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = false;
            // Note: movement is handled per-frame using keyboard state (below), not on single KEYDOWN events.

            std::shared_ptr<RotativeCamera> rotativeCamera = std::dynamic_pointer_cast<RotativeCamera>(camera);
            if (rotativeCamera) {
                if (event.type == SDL_MOUSEMOTION && !camera_locked) {
                    rotativeCamera->applyHorizontalRotation(-event.motion.xrel * mouseSensitivity);
                    rotativeCamera->applyVerticalRotation(-event.motion.yrel * mouseSensitivity);
                }
            }

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_l) {
                camera_locked = !camera_locked;
            }
        }

        // Compute delta time in seconds for smooth movement
        const uint32_t currentTicks = SDL_GetTicks();
        const float deltaTime = static_cast<float>(currentTicks - lastTicks) / 1000.0f;
        lastTicks = currentTicks;

        // Update FPS sampling once per second so the displayed number is stable
        fps_frames += 1;
        fps_accumulator += deltaTime;
        if (fps_accumulator >= 1.0f) {
            fps_display = static_cast<float>(fps_frames) / fps_accumulator;
            ms_display = (fps_accumulator / static_cast<float>(fps_frames)) * 1000.0f;
            fps_frames = 0;
            fps_accumulator = 0.0f;
        }

        // Apply continuous movement based on keyboard state
        std::shared_ptr<MovableCamera> movableCamera = std::dynamic_pointer_cast<MovableCamera>(camera);
        if (movableCamera && !camera_locked) {
            const Uint8* keyState = SDL_GetKeyboardState(NULL);
            if (keyState[SDL_SCANCODE_W]) {
                movableCamera->applyMovement(movableCamera->getOrientation(), moveUnitPerSecond * deltaTime);
            }
            if (keyState[SDL_SCANCODE_S]) {
                movableCamera->applyMovement(movableCamera->getOrientation(), -moveUnitPerSecond * deltaTime);
            }
            if (keyState[SDL_SCANCODE_A]) {
                movableCamera->applyMovement(glm::normalize(glm::cross(glm::vec3(0, 1, 0), camera->getOrientation())), moveUnitPerSecond * deltaTime);
            }
            if (keyState[SDL_SCANCODE_D]) {
                movableCamera->applyMovement(glm::normalize(glm::cross(glm::vec3(0, 1, 0), camera->getOrientation())), -moveUnitPerSecond * deltaTime);
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Imgui");
        ImGui::Text("This is some very useful text.");
        {
            std::ostringstream oss;
            oss.imbue(std::locale::classic());
            oss.setf(std::ios::fixed); oss.precision(1);
            oss << "FPS: " << fps_display << " (" << ms_display << " ms)";
            ImGui::TextUnformatted(oss.str().c_str());
        }

        // Light toggles
        if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushID("Lights");

            if (ImGui::Checkbox("Enable Sun", &sun_enabled)) {
                if (sun_enabled) {
                    scene->setDirectionalLight("Sun", sun_default);
                } else {
                    scene->removeDirectionalLight("Sun");
                }
            }

            if (ImGui::Checkbox("Enable Ambient", &ambient_enabled)) {
                if (ambient_enabled) {
                    scene->setAmbientLight(ambient_default);
                } else {
                    scene->removeAmbientLight();
                }
            }

            if (ImGui::Checkbox("Enable Spot", &spot_enabled)) {
                if (spot_enabled) {
                    scene->setConeLight("Spot", spot_default);
                } else {
                    scene->removeConeLight("Spot");
                }
            }

            if (ImGui::Checkbox("Enable Player Spot", &player_spot_enabled)) {
                if (player_spot_enabled) {
                    scene->setConeLight("PlayerSpot", player_spot_default);
                } else {
                    scene->removeConeLight("PlayerSpot");
                }
            }

            ImGui::PopID();
        }

        // Directional light (Sun) controls in its own panel
        auto sun_ptr = scene->getDirectionalLight("Sun");
        DirectionalLight new_sun = sun_ptr ? *sun_ptr : sun_default;
        if (sun_ptr) {
            if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::PushID("Sun");
                // direction
                {
                    const glm::vec3 curDir = sun_ptr->getDirection();
                    float dir[3] = { curDir.x, curDir.y, curDir.z };
                    if (ImGui::SliderFloat3("Direction", dir, -1.0f, 1.0f)) {
                        new_sun.setDirection(glm::vec3(dir[0], dir[1], dir[2]));
                    }
                }

                // color
                {
                    const glm::vec3 curCol = sun_ptr->getColor();
                    float col[3] = { curCol.r, curCol.g, curCol.b };
                    if (ImGui::ColorEdit3("Color", col)) {
                        new_sun.setColor(glm::vec3(col[0], col[1], col[2]));
                    }
                }
                // intensity
                {
                    float intensity = sun_ptr->getIntensity();
                    if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f)) {
                        new_sun.setIntensity(intensity);
                    }
                }
                ImGui::PopID();
            }
            scene->setDirectionalLight("Sun", new_sun);
        }

        // Ambient light controls in its own panel
        auto ambient_ptr = scene->getAmbientLight();
        AmbientLight new_ambient = ambient_ptr ? *ambient_ptr : ambient_default;
        if (ambient_ptr) {
            if (ImGui::CollapsingHeader("Ambient", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::PushID("Ambient");
                // color
                {
                    const glm::vec3 curCol = ambient_ptr->getColor();
                    float col[3] = { curCol.r, curCol.g, curCol.b };
                    if (ImGui::ColorEdit3("Color", col)) {
                        new_ambient.setColor(glm::vec3(col[0], col[1], col[2]));
                    }
                }
                // intensity
                {
                    float intensity = ambient_ptr->getIntensity();
                    if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f)) {
                        new_ambient.setIntensity(intensity);
                    }
                }
                ImGui::PopID();
            }
            scene->setAmbientLight(new_ambient);
        }

        // Cone light (Spot) controls in its own panel
        auto spot_ptr = scene->getConeLight("Spot");
        ConeLight new_spot = spot_ptr ? *spot_ptr : spot_default;
        if (spot_ptr) {
            if (ImGui::CollapsingHeader("Spot", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::PushID("Spot");
                // position
                {
                    const glm::vec3 curPos = spot_ptr->getPosition();
                    float pos[3] = { curPos.x, curPos.y, curPos.z };
                    if (ImGui::InputFloat3("Position", pos)) {
                        new_spot.setPosition(glm::vec3(pos[0], pos[1], pos[2]));
                    }
                }

                // direction
                {
                    const glm::vec3 curDir = spot_ptr->getDirection();
                    float dir[3] = { curDir.x, curDir.y, curDir.z };
                    if (ImGui::SliderFloat3("Direction", dir, -1.0f, 1.0f)) {
                        new_spot.setDirection(glm::vec3(dir[0], dir[1], dir[2]));
                    }
                }

                // angle (degrees)
                {
                    float deg = glm::degrees(spot_ptr->getAngleRadians());
                    if (ImGui::SliderFloat("Angle", &deg, 0.1f, 90.0f)) {
                        new_spot.setAngleRadians(glm::radians(deg));
                    }
                }

                // color
                {
                    const glm::vec3 curCol = spot_ptr->getColor();
                    float col[3] = { curCol.r, curCol.g, curCol.b };
                    if (ImGui::ColorEdit3("Color", col)) {
                        new_spot.setColor(glm::vec3(col[0], col[1], col[2]));
                    }
                }

                // intensity
                {
                    float intensity = spot_ptr->getIntensity();
                    if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f)) {
                        new_spot.setIntensity(intensity);
                    }
                }
                ImGui::PopID();
            }
            scene->setConeLight("Spot", new_spot);
        }

        // Player-following Cone light (Spot) controls in its own panel
        auto player_spotlight_ptr = scene->getConeLight("PlayerSpot");
        if (player_spotlight_ptr) {
            ConeLight new_player_spotlight = *player_spotlight_ptr;
            
            // Force the player spot to follow the camera each frame
            new_player_spotlight.setPosition(camera->getCameraPosition());
            new_player_spotlight.setDirection(camera->getOrientation());

            /*
            new_player_spotlight.setZNear(spot_default.getZNear());
            new_player_spotlight.setZFar(spot_default.getZFar());
            */

            if (ImGui::CollapsingHeader("Player Spot", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::PushID("PlayerSpot");

                // position (read-only)
                {
                    const glm::vec3 curPos = new_player_spotlight.getPosition();
                    ImGui::Text("Position: %.2f, %.2f, %.2f", curPos.x, curPos.y, curPos.z);
                }

                // direction (read-only)
                {
                    const glm::vec3 curDir = new_player_spotlight.getDirection();
                    ImGui::Text("Direction: %.2f, %.2f, %.2f", curDir.x, curDir.y, curDir.z);
                }

                // angle (degrees)
                {
                    float deg = glm::degrees(new_player_spotlight.getAngleRadians());
                    if (ImGui::SliderFloat("Angle", &deg, 0.1f, 90.0f)) {
                        new_player_spotlight.setAngleRadians(glm::radians(deg));
                    }
                }

                // color
                {
                    const glm::vec3 curCol = new_player_spotlight.getColor();
                    float col[3] = { curCol.r, curCol.g, curCol.b };
                    if (ImGui::ColorEdit3("Color", col)) {
                        new_player_spotlight.setColor(glm::vec3(col[0], col[1], col[2]));
                    }
                }

                // intensity
                {
                    float intensity = new_player_spotlight.getIntensity();
                    if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 100.0f)) {
                        new_player_spotlight.setIntensity(intensity);
                    }
                }

                ImGui::PopID();
            }

            scene->setConeLight("PlayerSpot", new_player_spotlight);
        }

        ImGui::End();

        // Render the scene
        const auto render_scene = scene.get();
        if (!render_scene) {
            std::cerr << "No scene to render!" << std::endl;
        } else {
            pipeline->render(*render_scene);
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
