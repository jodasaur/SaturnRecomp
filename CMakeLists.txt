cmake_minimum_required(VERSION 3.20)
project(SaturnRecomp LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(SATURNRECOMP_BUILD_GUI "Build the SDL2/Vulkan frontend" OFF)
option(SATURNRECOMP_BUILD_TESTS "Build and register existing tests" ON)

if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "SaturnRecomp currently requires GCC or Clang (Windows: MinGW-w64)")
endif()

add_library(saturn_build_options INTERFACE)
target_compile_features(saturn_build_options INTERFACE c_std_11)
target_compile_options(saturn_build_options INTERFACE -Wall -Wextra)
if(NOT WIN32)
    target_compile_definitions(saturn_build_options INTERFACE _POSIX_C_SOURCE=200809L)
    target_link_libraries(saturn_build_options INTERFACE m)
endif()
if(MINGW)
    # The runtime and several tests keep large Saturn snapshots on the stack.
    target_link_options(saturn_build_options INTERFACE -Wl,--stack,67108864)
endif()

add_library(sh2_decoder STATIC)
target_sources(sh2_decoder PRIVATE external/sh2-recomp-core/common/sh2_decoder.c)
target_include_directories(sh2_decoder PUBLIC external/sh2-recomp-core/common)
target_link_libraries(sh2_decoder PUBLIC saturn_build_options)

add_library(saturn_disc STATIC)
target_sources(saturn_disc PRIVATE recompiler/src/disc.c recompiler/src/game_config.c)
target_include_directories(saturn_disc PUBLIC recompiler/include)
target_link_libraries(saturn_disc PUBLIC saturn_build_options)

add_library(saturn_core STATIC)
target_sources(saturn_core PRIVATE
    runner/src/bus.c runner/src/scu_dsp.c runner/src/sh2_interp.c
    runner/src/bios.c runner/src/cdblock.c runner/src/smpc.c
    runner/src/vdp1.c runner/src/vdp2.c runner/src/debugview.c runner/src/png.c
    runner/src/m68k.c runner/src/m68k_bus.c runner/src/scsp.c
    runner/src/scsp_dsp.c runner/src/sound.c)
target_include_directories(saturn_core PUBLIC runner/include)
target_link_libraries(saturn_core PUBLIC sh2_decoder saturn_disc saturn_build_options)

add_library(saturn_settings STATIC)
target_sources(saturn_settings PRIVATE runner/src/runtime_settings.c)
target_include_directories(saturn_settings PUBLIC runner/include)
target_link_libraries(saturn_settings PUBLIC saturn_build_options)

add_executable(saturnrecomp recompiler/src/main.c)
target_link_libraries(saturnrecomp PRIVATE saturn_disc sh2_decoder)
add_executable(saturnboot runner/src/boot.c)
target_link_libraries(saturnboot PRIVATE saturn_core saturn_settings)

if(SATURNRECOMP_BUILD_GUI)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(Vulkan REQUIRED)
    find_program(SATURNRECOMP_GLSLC glslc REQUIRED)

    set(saturn_shaders)
    foreach(shader vdp1.comp vdp2.comp postprocess.comp game_overlay.vert game_overlay.frag)
        set(output "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv")
        add_custom_command(OUTPUT "${output}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            COMMAND "${SATURNRECOMP_GLSLC}" "${CMAKE_CURRENT_SOURCE_DIR}/runner/shaders/${shader}" -O -o "${output}"
            DEPENDS "runner/shaders/${shader}" VERBATIM)
        list(APPEND saturn_shaders "${output}")
    endforeach()
    add_custom_target(saturn_shaders ALL DEPENDS ${saturn_shaders})

    add_library(saturn_overlay STATIC runner/src/game_overlay.c)
    target_link_libraries(saturn_overlay PUBLIC saturn_settings SDL2::SDL2)
    add_library(saturn_vulkan STATIC runner/src/vulkan_renderer.c runner/src/game_overlay_vk.c)
    target_link_libraries(saturn_vulkan PUBLIC saturn_core saturn_overlay Vulkan::Vulkan)
    add_dependencies(saturn_vulkan saturn_shaders)

    add_executable(saturnwin runner/src/window.c)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(saturnwin PRIVATE SDL2::SDL2main)
    endif()
    target_link_libraries(saturnwin PRIVATE saturn_vulkan)
    add_custom_command(TARGET saturnwin POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders" "$<TARGET_FILE_DIR:saturnwin>/shaders"
        VERBATIM)
    if(WIN32)
        add_custom_command(TARGET saturnwin POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:saturnwin>" VERBATIM)
    endif()
endif()

if(SATURNRECOMP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
