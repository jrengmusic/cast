#[[*****************************************************************************
                        Codegen Annotated Source of Truth
————————————————————————————————————————————————————————————————————————————————

            ░░████████████░░████████████░░████████████░░████████████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████        ░░████  ░░████░░████            ░░████
            ░░████        ░░████████████░░████████████    ░░████
            ░░████        ░░████  ░░████        ░░████    ░░████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████████████░░████  ░░████░░████████████    ░░████

————————————————————————————————————————————————————————————————————————————————
                         FOR YOUR EYES ONLY, DO NOT EDIT
******************************************************************************]]

cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target")

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
elseif(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project("cast"
    VERSION "0.1.0"
    LANGUAGES C CXX
    DESCRIPTION "Code Annotated Source of Truth
████████████░░████████████░░████████████░░████████████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████░░        ████░░  ████░░████░░            ████░░
████░░        ████████████░░████████████░░    ████░░
████░░        ████░░  ████░░        ████░░    ████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████████████░░████░░  ████░░████████████░░    ████░░"
)

message(STATUS "${PROJECT_DESCRIPTION}")

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE CAST_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "CAST: configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

set(CAST_CLANG_WARNING_FLAGS
-Wno-float-equal
-Wno-sign-conversion
-Wno-switch-enum
-Wno-unused-parameter
-Wno-implicit-float-conversion
-Wno-shadow
)

set(CAST_MSVC_WARNING_FLAGS
/wd4100
/wd4244
/wd4456
/permissive-
/Zc:rvalueCast
/W4
/FC
)

if(MSVC)
    set(CAST_WARNING_FLAGS ${CAST_MSVC_WARNING_FLAGS})
else()
    set(CAST_WARNING_FLAGS ${CAST_CLANG_WARNING_FLAGS})
endif()

add_compile_options(${CAST_WARNING_FLAGS})

set(CAST_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE")
set(CAST_USER_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../jam")

set(CAST_JUCE_VERSION 8.0.14)

find_package(Git REQUIRED)

if(WIN32)
    set(CAST_JUCE_TEMP_DIR "$ENV{TEMP}")
elseif(DEFINED ENV{TMPDIR})
    set(CAST_JUCE_TEMP_DIR "$ENV{TMPDIR}")
else()
    set(CAST_JUCE_TEMP_DIR "/tmp")
endif()

set(CAST_PATCHED_JUCE_PATH "${CAST_JUCE_TEMP_DIR}/juce-patched")

if(NOT EXISTS "${CAST_PATCHED_JUCE_PATH}")
    message(STATUS "CAST: using JUCE at ${CAST_JUCE_PATH}")
    set(CAST_PATCHED_JUCE_PATH "${CAST_JUCE_PATH}")
endif()

add_subdirectory("${CAST_PATCHED_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")

get_directory_property(CAST_FOUND_JUCE_VERSION DIRECTORY "${CAST_PATCHED_JUCE_PATH}" DEFINITION JUCE_VERSION)

if(NOT CAST_FOUND_JUCE_VERSION VERSION_EQUAL CAST_JUCE_VERSION)
    message(FATAL_ERROR "CAST: JUCE ${CAST_FOUND_JUCE_VERSION} found, ${CAST_JUCE_VERSION} required — exact match")
endif()

message(STATUS "CAST: JUCE ${CAST_FOUND_JUCE_VERSION} found at ${CAST_PATCHED_JUCE_PATH}")

juce_add_module("${CAST_USER_MODULE_PATH}/jam_core")    # JAM Core
juce_add_module("${CAST_USER_MODULE_PATH}/jam_markdown")# Clean-room native CommonMark + GFM markdown parsing/rendering

message(STATUS "CAST: user modules added from ${CAST_USER_MODULE_PATH}")

juce_add_console_app(${PROJECT_NAME}
    PRODUCT_NAME "cast"
    VERSION "0.1.0"
    COMPANY_NAME "JRENG"
    COMPANY_WEBSITE "https://jrengmusic.com"
    COMPANY_EMAIL "info@jrengmusic.com"
)

juce_generate_juce_header(${PROJECT_NAME})

file(GLOB_RECURSE CAST_SOURCE_FILES CONFIGURE_DEPENDS
${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
)

target_sources(${PROJECT_NAME} PRIVATE ${CAST_SOURCE_FILES})

target_compile_definitions(${PROJECT_NAME} PUBLIC
    CAST_COMMIT="${CAST_COMMIT_HASH}"
DONT_SET_USING_JUCE_NAMESPACE=1
JUCE_DONT_DECLARE_PROJECTINFO=1
)

target_include_directories(${PROJECT_NAME} PRIVATE
${CAST_USER_MODULE_PATH}
${CMAKE_CURRENT_SOURCE_DIR}/Source/generated
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
juce_core
jam_core
jam_markdown
    PUBLIC
        juce::juce_recommended_config_flags
)

target_compile_options(${PROJECT_NAME} PRIVATE ${CAST_WARNING_FLAGS})

file(GLOB CAST_BINARY_FILES CONFIGURE_DEPENDS
Source/HELP.md
Source/style.css
Source/resources/cast-output.md
${CAST_USER_MODULE_PATH}/resources/svg/*.svg
)

juce_add_binary_data(${PROJECT_NAME}_BinaryData
    NAMESPACE "BinaryData"
    SOURCES
        ${CAST_BINARY_FILES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_BinaryData)

include(CheckIPOSupported)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    check_ipo_supported(RESULT CAST_IPO_SUPPORTED OUTPUT CAST_IPO_ERROR)

    if(CAST_IPO_SUPPORTED)
        set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "CAST: LTO enabled")
    else()
        message(WARNING "CAST: LTO not supported — ${CAST_IPO_ERROR}")
    endif()

    if(APPLE OR UNIX)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "CAST: strip"
        )
    elseif(MSVC)
        target_link_options(${PROJECT_NAME} PRIVATE /OPT:REF /OPT:ICF)
    endif()
endif()

message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "CAST: ${PROJECT_NAME} configured")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

if(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "CAST: signing $<TARGET_FILE_NAME:${PROJECT_NAME}>..."
        COMMAND xattr -cr $<TARGET_FILE:${PROJECT_NAME}>
        COMMAND codesign --force --options runtime
                --entitlements "${CMAKE_CURRENT_SOURCE_DIR}/entitlements.plist"
                --sign "Developer ID Application: Bayu Ardianto (9BDSN9TDX3)"
                $<TARGET_FILE:${PROJECT_NAME}>
        COMMAND codesign --verify --verbose $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "CAST: codesign"
        VERBATIM
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "CAST: notarizing $<TARGET_FILE_NAME:${PROJECT_NAME}>..."
        COMMAND ditto -c -k --keepParent $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_FILE:${PROJECT_NAME}>.notarize.zip
        COMMAND xcrun notarytool submit $<TARGET_FILE:${PROJECT_NAME}>.notarize.zip
                --keychain-profile "notary"
                --wait
        COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:${PROJECT_NAME}>.notarize.zip
        COMMENT "CAST: notarize"
        VERBATIM
    )
endif()
