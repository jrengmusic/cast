cmake_minimum_required(VERSION 3.25)

set(JAM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../jam")
list(APPEND CMAKE_MODULE_PATH "${JAM_ROOT}/cmake")
include(BuildSetup)

project(cast
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "Universal headless codegen"
)

execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE CAST_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_app(
    TARGET_NAME  cast_App
    PRODUCT_NAME cast
    VERSION      "${PROJECT_VERSION}"
    MODULES      juce_core
    # P8b: full jam surface for generated-header validation — slim back after doxygen pass
    JAM_MODULES  jam_core
                 jam_style
                 jam_terminal
                 jam_markdown
                 jam_animation
                 jam_audio_devices
                 jam_clap
                 jam_data_structures
                 jam_debug
                 jam_dsp
                 jam_freetype
                 jam_fuzzy
                 jam_graphics
                 jam_gui
                 jam_lua
                 jam_subprocess
                 jam_vulkan
                 jam_web
)

target_include_directories(cast_App PRIVATE
    "${JAM_ROOT}"
    "${JUCE_PATH}/modules/juce_graphics/fonts/harfbuzz"
)

target_compile_definitions(cast_App PRIVATE
    CAST_COMMIT="${CAST_COMMIT_HASH}"
)

juce_add_binary_data(
    cast_BinaryData 
    SOURCES Source/HELP.md 
            Source/style.css
            Source/banner/cast-output.md
)

target_link_libraries(cast_App PRIVATE cast_BinaryData)

message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "Configuration complete!")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
