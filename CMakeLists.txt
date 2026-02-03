cmake_minimum_required(VERSION 3.10)
project(cg_univr_project)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(cg_lib STATIC
    ./code/OpenGL.cpp
    ./code/Buffer.cpp
    ./code/Mesh.cpp
    ./code/Material.cpp
    ./code/Texture.cpp
    ./code/Scene.cpp
    ./code/Shader.cpp
    ./code/Program.cpp
    ./code/Pipeline.cpp
    ./code/Framebuffer.cpp
    ./code/RenderQuad.cpp
    ./code/Pipelines/UnshadowedPipeline.cpp
    ./code/Pipelines/ShadowedPipeline.cpp
    ./code/Camera/Camera.cpp
    ./code/Camera/MovableCamera.cpp
    ./code/Camera/ProjectionCamera.cpp
    ./code/Camera/OrthoCamera.cpp
    ./code/Camera/RotativeCamera.cpp
    ./code/Camera/SpectatorCamera.cpp
    ./code/Camera/OrthoSpectatorCamera.cpp
    ./code/Light/Light.cpp
    ./code/Light/AmbientLight.cpp
    ./code/Light/DirectionalLight.cpp
    ./code/Light/ConeLight.cpp
    ./include/glad/glad.c
    ./include/imgui/imgui.cpp
    ./include/imgui/imgui_draw.cpp
    ./include/imgui/imgui_tables.cpp
    ./include/imgui/imgui_widgets.cpp
    ./include/imgui/backends/imgui_impl_sdl2.cpp
    ./include/imgui/backends/imgui_impl_opengl3.cpp
)

# --- Shader compilation to SPIR-V ---
set(SHADERS_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADERS_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADERS_OUT_DIR})

# Path for the generated symbols header (always defined so build doesn't fail when glslangValidator is absent)
set(EMBED_SYMBOLS_HEADER ${SHADERS_OUT_DIR}/embedded_shaders_symbols.h)
# Ensure a placeholder header exists at configure time so compilers
# that build targets immediately can find the include file.
file(WRITE ${EMBED_SYMBOLS_HEADER} "// Placeholder generated at configure time by CMake\n#pragma once\n\n")

find_program(GLSLANG_VALIDATOR glslangValidator)
if(GLSLANG_VALIDATOR)
    message(STATUS "Found glslangValidator: ${GLSLANG_VALIDATOR}")
    set(SHADER_FILES
        unshadowed.vert
        unshadowed.geom
        unshadowed.frag
        quad.vert
        depth_only.vert
        depth_only.frag
        ssao.frag
        ssao_blur.frag
        ambient_lighting.frag
        directional_lighting.frag
        cone_lighting.frag
        unshadowed_directional_lighting.frag
        post.frag
        tone_mapping.frag
    )

    set(SPIRV_TARGETS)
    # Preprocessor defines passed to the shader compiler. Keep in sync with code/settings.hpp
    set(SHADER_DEFINES "-DDIRECTIONAL_LIGHT_COUNT=2")
    set(EMBED_EXTERN_DECLS)
    foreach(f IN LISTS SHADER_FILES)
        get_filename_component(fname ${f} NAME_WE)
        get_filename_component(fext ${f} EXT)
        # build output name: <name>.<ext>.spv (e.g. unshadowed.vert.spv)
        set(out ${SHADERS_OUT_DIR}/${fname}${fext}.spv)
        add_custom_command(
            OUTPUT ${out}
            # Generate SPIR-V targeting OpenGL's semantics
            COMMAND ${GLSLANG_VALIDATOR} -G ${SHADER_DEFINES} -o ${out} ${SHADERS_SRC_DIR}/${f}
            DEPENDS ${SHADERS_SRC_DIR}/${f}
            COMMENT "Compiling shader ${f} -> ${out}"
            VERBATIM
        )
        list(APPEND SPIRV_TARGETS ${out})
    endforeach()

    add_custom_target(shaders ALL DEPENDS ${SPIRV_TARGETS})
    add_dependencies(cg_lib shaders)
    # Embed generated SPIR-V into C++ sources so the application can be standalone
    set(EMBEDDED_SRCS)
    set(EMBED_SYMBOL_DEFINES)
    set(EMBED_EXTERN_DECLS)
    foreach(out IN LISTS SPIRV_TARGETS)
        get_filename_component(fname ${out} NAME)
        string(REGEX REPLACE "\\.spv$" "" symbol ${fname})
        set(embed_cpp ${SHADERS_OUT_DIR}/embedded_${symbol}.c)
        # Compute a clean symbol name (basename with dots -> underscores) and append _spv
        get_filename_component(basename ${out} NAME)
        string(REGEX REPLACE "\\.spv$" "" basename_no_spv ${basename})
        string(REPLACE "." "_" short_base ${basename_no_spv})
        set(xxd_sym "${short_base}_spv")
        # Use xxd -i -n to produce a C array with the chosen symbol name (no path in symbol)
        add_custom_command(
            OUTPUT ${embed_cpp}
            COMMAND /bin/sh -c "xxd -i -n ${xxd_sym} ${out} > ${embed_cpp}"
            DEPENDS ${out}
            COMMENT "Embedding SPIR-V ${out} -> ${embed_cpp} (symbol: ${xxd_sym})"
            VERBATIM
        )
        list(APPEND EMBEDDED_SRCS ${embed_cpp})
        # Record defines and externs for the generated symbol
        set(short_sym "${xxd_sym}")
        list(APPEND EMBED_SYMBOL_DEFINES "#define ${short_sym} ${xxd_sym}")
        list(APPEND EMBED_SYMBOL_DEFINES "#define ${short_sym}_len ${xxd_sym}_len")
        list(APPEND EMBED_EXTERN_DECLS "extern unsigned char ${xxd_sym}[];")
        list(APPEND EMBED_EXTERN_DECLS "extern unsigned int ${xxd_sym}_len;")
    endforeach()

    # Also embed the original GLSL source files as C arrays so we can fallback to compiled-in GLSL
    foreach(f IN LISTS SHADER_FILES)
        get_filename_component(fname ${f} NAME_WE)
        get_filename_component(fext ${f} EXT)
        set(embed_src_c ${SHADERS_OUT_DIR}/embedded_src_${fname}${fext}.c)
        # Compute a clean symbol name for the source: <name>_<ext_no_dot>_glsl and pass it to xxd
        string(LENGTH "${fext}" _fext_len)
        math(EXPR _fext_body_len "${_fext_len} - 1")
        if(_fext_body_len GREATER 0)
            string(SUBSTRING "${fext}" 1 ${_fext_body_len} fext_no_dot)
        else()
            set(fext_no_dot "")
        endif()
        set(xxd_src_sym "${fname}_${fext_no_dot}_glsl")
        add_custom_command(
            OUTPUT ${embed_src_c}
            COMMAND /bin/sh -c "xxd -i -n ${xxd_src_sym} ${SHADERS_SRC_DIR}/${f} > ${embed_src_c}"
            DEPENDS ${SHADERS_SRC_DIR}/${f}
            COMMENT "Embedding shader source ${SHADERS_SRC_DIR}/${f} -> ${embed_src_c} (symbol: ${xxd_src_sym})"
            VERBATIM
        )
        list(APPEND EMBEDDED_SRCS ${embed_src_c})
        # Map a short name like <name>_<ext>_glsl to the generated symbol and record externs
        set(short_src_sym "${fname}_${fext_no_dot}_glsl")
        list(APPEND EMBED_SYMBOL_DEFINES "#define ${short_src_sym} ${xxd_src_sym}")
        list(APPEND EMBED_SYMBOL_DEFINES "#define ${short_src_sym}_len ${xxd_src_sym}_len")
        list(APPEND EMBED_EXTERN_DECLS "extern unsigned char ${xxd_src_sym}[];")
        list(APPEND EMBED_EXTERN_DECLS "extern unsigned int ${xxd_src_sym}_len;")
    endforeach()

    # Ensure embedded sources are generated before building the executable
    add_custom_target(embedded_shaders ALL DEPENDS ${EMBEDDED_SRCS})
    add_dependencies(cg_lib embedded_shaders)

    # Add embedded sources to the project target
    target_sources(cg_lib PRIVATE ${EMBEDDED_SRCS})

    # expose the generated shader output dir to the compiler's include path so we can include the generated header
    target_include_directories(cg_lib PRIVATE ${SHADERS_OUT_DIR})

    # Generate a header that maps stable short symbol names to the actual symbol names emitted by xxd
    set(EMBED_SYMBOLS_HEADER ${SHADERS_OUT_DIR}/embedded_shaders_symbols.h)
    file(WRITE ${EMBED_SYMBOLS_HEADER} "// Generated by CMake - do not edit\n#pragma once\n\n")
    # Output the #defines mapping short names to generated symbol names
    foreach(def IN LISTS EMBED_SYMBOL_DEFINES)
        file(APPEND ${EMBED_SYMBOLS_HEADER} "${def}\n")
    endforeach()

    # Also declare the actual generated symbols as extern so C++ files can reference them.
    file(APPEND ${EMBED_SYMBOLS_HEADER} "\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
    foreach(decl IN LISTS EMBED_EXTERN_DECLS)
        file(APPEND ${EMBED_SYMBOLS_HEADER} "${decl};\n")
    endforeach()
    file(APPEND ${EMBED_SYMBOLS_HEADER} "\n#ifdef __cplusplus\n}\n#endif\n")
else()
    message(WARNING "glslangValidator not found; SPIR-V shaders will not be generated. Place .spv files in ${SHADERS_OUT_DIR} or install glslangValidator.")
    # Create a minimal placeholder header so compilation that includes
    # embedded_shaders_symbols.h does not fail when shaders aren't embedded.
    file(WRITE ${EMBED_SYMBOLS_HEADER} "// Placeholder generated by CMake - glslangValidator not found\n#pragma once\n\n")
endif()

# Provide the shader output directory path to the code so it can load compiled SPIR-V at runtime
target_compile_definitions(cg_lib PRIVATE SHADERS_DIR=\"${SHADERS_OUT_DIR}\" SHADER_SRC_DIR=\"${SHADERS_SRC_DIR}\")

# Always expose the shader output directory to the compiler include path so
# `#include "embedded_shaders_symbols.h"` works even when glslangValidator
# is not present and the file was created as a placeholder at configure time.
target_include_directories(cg_lib PRIVATE ${SHADERS_OUT_DIR})

# Prefer modern SDL2 imported target if available, otherwise fall back to FindSDL2
find_package(SDL2 CONFIG QUIET)
if (TARGET SDL2::SDL2)
    set(SDL2_LIBRARIES SDL2::SDL2)
    set(SDL2_INCLUDE_DIRS "")
else()
    find_package(SDL2 REQUIRED)
endif()

# Set CMP0072 explicitly to use modern OpenGL (GLVND) implementation
cmake_policy(SET CMP0072 NEW)
set(OpenGL_GL_PREFERENCE "GLVND")

find_package(glm REQUIRED)

# Include GLM
include_directories(${glm_INCLUDE_DIRS})
link_directories(${glm_LIBRARY_DIRS})
add_definitions(${glm_DEFINITIONS})

# Include directories for GLAD, GLFW, and other libraries
target_include_directories(cg_lib PUBLIC
    ./include
    ${SDL2_INCLUDE_DIRS}
    ${glm_INCLUDE_DIRS}
    ./include/imgui
    ./include/KHR
)

# Add dds_loader subdirectory (builds `dds_loader` target)
add_subdirectory(${CMAKE_SOURCE_DIR}/include/dds_loader)

# Find and link Assimp (use system installation)
find_package(assimp REQUIRED)
message(STATUS "Assimp: ${assimp_VERSION}")

if (TARGET assimp::assimp)
    target_link_libraries(cg_lib PUBLIC assimp::assimp)
else()
    if (DEFINED ASSIMP_INCLUDE_DIRS)
        target_include_directories(cg_lib PUBLIC ${ASSIMP_INCLUDE_DIRS})
    elseif (DEFINED ASSIMP_INCLUDE_DIR)
        target_include_directories(cg_lib PUBLIC ${ASSIMP_INCLUDE_DIR})
    endif()

    if (DEFINED ASSIMP_LIBRARIES)
        target_link_libraries(cg_lib PUBLIC ${ASSIMP_LIBRARIES})
    elseif (DEFINED ASSIMP_LIBRARY)
        target_link_libraries(cg_lib PUBLIC ${ASSIMP_LIBRARY})
    else()
        message(FATAL_ERROR "assimp found but no usable variables/targets provided by the package")
    endif()
endif()

# Link the dds_loader library built from include/dds_loader
target_link_libraries(cg_lib PUBLIC dds_loader)

# Link libraries depending on USE_GLES
find_library(GLES_LIB NAMES GLESv2 GLES)
find_library(EGL_LIB NAMES EGL)
if(NOT GLES_LIB)
    message(FATAL_ERROR "OpenGL ES library not found. Install libgles or adjust CMakeLists.")
endif()

# Tell ImGui backend to compile ES3 code-path
target_compile_definitions(cg_lib PUBLIC IMGUI_IMPL_OPENGL_ES3 GLAD_GLES2_USE_SYSTEM_EGL)
target_link_libraries(cg_lib PUBLIC ${SDL2_LIBRARIES} ${GLES_LIB} ${EGL_LIB} ${glm_LIBRARIES})

include_directories(
    ${CMAKE_SOURCE_DIR}/code
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/stb
    ${CMAKE_SOURCE_DIR}/include/glad
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/dds_loader
)

# main project executable
add_executable(project ./code/main.cpp)
target_link_libraries(project PRIVATE cg_lib)

# Da qui in poi uso GLFW

# Include GLFW
find_package(glfw3 REQUIRED)

add_executable(esercizio01 ./esercizi/esercizio01.cpp)
target_link_libraries(esercizio01 PRIVATE glfw cg_lib)

add_executable(esercizio02 ./esercizi/esercizio02.cpp)
target_link_libraries(esercizio02 PRIVATE glfw cg_lib)

add_executable(esercizio03 ./esercizi/esercizio03.cpp)
target_link_libraries(esercizio03 PRIVATE glfw cg_lib)

add_executable(esercizio04 ./esercizi/esercizio04.cpp)
target_link_libraries(esercizio04 PRIVATE glfw cg_lib)

add_executable(esercizio05 ./esercizi/esercizio05.cpp)
target_link_libraries(esercizio05 PRIVATE glfw cg_lib)

add_executable(esercizio06 ./esercizi/esercizio06.cpp)
target_link_libraries(esercizio06 PRIVATE glfw cg_lib)

add_executable(esercizio07 ./esercizi/esercizio07.cpp)
target_link_libraries(esercizio07 PRIVATE glfw cg_lib)

add_executable(esercizio08 ./esercizi/esercizio08.cpp)
target_link_libraries(esercizio08 PRIVATE glfw cg_lib)

add_executable(esercizio09 ./esercizi/esercizio09.cpp)
target_link_libraries(esercizio09 PRIVATE glfw cg_lib)

add_executable(esercizio10 ./esercizi/esercizio10.cpp)
target_link_libraries(esercizio10 PRIVATE glfw cg_lib)

add_executable(esercizio11 ./esercizi/esercizio11.cpp)
target_link_libraries(esercizio11 PRIVATE glfw cg_lib)

add_executable(esercizio12 ./esercizi/esercizio12.cpp)
target_link_libraries(esercizio12 PRIVATE glfw cg_lib)

add_executable(esercizio13 ./esercizi/esercizio13.cpp)
target_link_libraries(esercizio13 PRIVATE glfw cg_lib)

add_executable(esercizio14 ./esercizi/esercizio14.cpp)
target_link_libraries(esercizio14 PRIVATE glfw cg_lib)

add_executable(esercizio15 ./esercizi/esercizio15.cpp)
target_link_libraries(esercizio15 PRIVATE glfw cg_lib)

add_executable(esercizio16 ./esercizi/esercizio16.cpp)
target_link_libraries(esercizio16 PRIVATE glfw cg_lib)

add_executable(esercizio17 ./esercizi/esercizio17.cpp)
target_link_libraries(esercizio17 PRIVATE glfw cg_lib)

add_executable(esercizio18 ./esercizi/esercizio18.cpp)
target_link_libraries(esercizio18 PRIVATE glfw cg_lib)

add_executable(esercizio19 ./esercizi/esercizio19.cpp)
target_link_libraries(esercizio19 PRIVATE glfw cg_lib)

add_executable(esercizio20 ./esercizi/esercizio20.cpp)
target_link_libraries(esercizio20 PRIVATE glfw cg_lib)

add_executable(esercizio21 ./esercizi/esercizio21.cpp)
target_link_libraries(esercizio21 PRIVATE glfw cg_lib)

add_executable(esercizio22 ./esercizi/esercizio22.cpp)
target_link_libraries(esercizio22 PRIVATE glfw cg_lib)

add_executable(esercizio23 ./esercizi/esercizio23.cpp)
target_link_libraries(esercizio23 PRIVATE glfw cg_lib)

add_executable(esercizio24 ./esercizi/esercizio24.cpp)
target_link_libraries(esercizio24 PRIVATE glfw cg_lib)

add_executable(esercizio25 ./esercizi/esercizio25.cpp)
target_link_libraries(esercizio25 PRIVATE glfw cg_lib)

add_executable(esercizio26 ./esercizi/esercizio26.cpp)
target_link_libraries(esercizio26 PRIVATE glfw cg_lib)

add_executable(esercizio27 ./esercizi/esercizio27.cpp)
target_link_libraries(esercizio27 PRIVATE glfw cg_lib)

add_executable(esercizio28 ./esercizi/esercizio28.cpp)
target_link_libraries(esercizio28 PRIVATE glfw cg_lib)

add_executable(esercizio29 ./esercizi/esercizio29.cpp)
target_link_libraries(esercizio29 PRIVATE glfw cg_lib)

add_executable(esercizio30 ./esercizi/esercizio30.cpp)
target_link_libraries(esercizio30 PRIVATE glfw cg_lib)

add_executable(esercizio31 ./esercizi/esercizio31.cpp)
target_link_libraries(esercizio31 PRIVATE glfw cg_lib)

add_executable(esercizio32 ./esercizi/esercizio32.cpp)
target_link_libraries(esercizio32 PRIVATE glfw cg_lib)

add_executable(esercizio33 ./esercizi/esercizio33.cpp)
target_link_libraries(esercizio33 PRIVATE glfw cg_lib)

add_executable(esercizio34 ./esercizi/esercizio34.cpp)
target_link_libraries(esercizio34 PRIVATE glfw cg_lib)

add_executable(esercizio35 ./esercizi/esercizio35.cpp)
target_link_libraries(esercizio35 PRIVATE glfw cg_lib)

add_executable(esercizio36 ./esercizi/esercizio36.cpp)
target_link_libraries(esercizio36 PRIVATE glfw cg_lib)

add_executable(esercizio37 ./esercizi/esercizio37.cpp)
target_link_libraries(esercizio37 PRIVATE glfw cg_lib)

add_executable(esercizio38 ./esercizi/esercizio38.cpp)
target_link_libraries(esercizio38 PRIVATE glfw cg_lib)

add_executable(esercizio39 ./esercizi/esercizio39.cpp)
target_link_libraries(esercizio39 PRIVATE glfw cg_lib)

add_executable(esercizio40 ./esercizi/esercizio40.cpp)
target_link_libraries(esercizio40 PRIVATE glfw cg_lib)

add_executable(esercizio41 ./esercizi/esercizio41.cpp)
target_link_libraries(esercizio41 PRIVATE glfw cg_lib)

add_executable(esercizio42 ./esercizi/esercizio42.cpp)
target_link_libraries(esercizio42 PRIVATE glfw cg_lib)

add_executable(esercizio43 ./esercizi/esercizio43.cpp)
target_link_libraries(esercizio43 PRIVATE glfw cg_lib)

add_executable(esercizio44 ./esercizi/esercizio44.cpp)
target_link_libraries(esercizio44 PRIVATE glfw cg_lib)

add_executable(esercizio45 ./esercizi/esercizio45.cpp)
target_link_libraries(esercizio45 PRIVATE glfw cg_lib)

add_executable(esercizio46 ./esercizi/esercizio46.cpp)
target_link_libraries(esercizio46 PRIVATE glfw cg_lib)

add_executable(esercizio47 ./esercizi/esercizio47.cpp)
target_link_libraries(esercizio47 PRIVATE glfw cg_lib)

add_executable(esercizio48 ./esercizi/esercizio48.cpp)
target_link_libraries(esercizio48 PRIVATE glfw cg_lib)

add_executable(esercizio49 ./esercizi/esercizio49.cpp)
target_link_libraries(esercizio49 PRIVATE glfw cg_lib)

add_executable(esercizio50 ./esercizi/esercizio50.cpp)
target_link_libraries(esercizio50 PRIVATE glfw cg_lib)

add_executable(esercizio51 ./esercizi/esercizio51.cpp)
target_link_libraries(esercizio51 PRIVATE glfw cg_lib)

add_executable(esercizio52 ./esercizi/esercizio52.cpp)
target_link_libraries(esercizio52 PRIVATE glfw cg_lib)

add_executable(esercizio53 ./esercizi/esercizio53.cpp)
target_link_libraries(esercizio53 PRIVATE glfw cg_lib)

add_executable(esercizio54 ./esercizi/esercizio54.cpp)
target_link_libraries(esercizio54 PRIVATE glfw cg_lib)

add_executable(esercizio55 ./esercizi/esercizio55.cpp)
target_link_libraries(esercizio55 PRIVATE glfw cg_lib)

add_executable(esercizio56 ./esercizi/esercizio56.cpp)
target_link_libraries(esercizio56 PRIVATE glfw cg_lib)
