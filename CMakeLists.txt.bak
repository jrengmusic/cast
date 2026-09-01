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
    JAM_MODULES  jam_core
                 jam_markdown
    BINARY_FILES Source/HELP.md
                 Source/style.css
                 Source/resources/cast-output.md
)

target_include_directories(cast_App PRIVATE
    "${JAM_ROOT}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/generated"
)

target_compile_definitions(cast_App PRIVATE
    CAST_COMMIT="${CAST_COMMIT_HASH}"
    JUCE_DONT_DECLARE_PROJECTINFO=1
)

message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "Configuration complete!")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
