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
option(CAST_SIGN "Run the sign/pkg/notarize/staple chain on a Release build" ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target")

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
elseif(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project("cast"
    VERSION "0.1.0"
    LANGUAGES C CXX
    DESCRIPTION "Codegen Annotated Source of Truth"
)

message(STATUS "\n████████████░░████████████░░████████████░░████████████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████░░        ████░░  ████░░████░░            ████░░
████░░        ████████████░░████████████░░    ████░░
████░░        ████░░  ████░░        ████░░    ████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████████████░░████░░  ████░░████████████░░    ████░░")

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE CAST_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "CAST: configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

if(APPLE)
    set(CAST_RELEASE_COMPILE_FLAGS
-Wno-shadow;-Wno-unused-parameter;-Wno-float-equal;-Wno-sign-conversion;-Wno-switch-enum;-Wno-implicit-float-conversion;-O3;-flto=thin
    )
    set(CAST_RELEASE_LINK_FLAGS
-dead_strip
    )
    set(CAST_DEBUG_COMPILE_FLAGS
-Wno-shadow;-Wno-unused-parameter;-Wno-float-equal;-Wno-sign-conversion;-Wno-switch-enum;-Wno-implicit-float-conversion;-O0;-g
    )
elseif(WIN32)
    set(CAST_RELEASE_COMPILE_FLAGS
/wd4456;/wd4100;/wd4244;/permissive-;/Zc:rvalueCast;/W4;/FC;/O2;/GL
    )
    set(CAST_RELEASE_LINK_FLAGS
/LTCG;/OPT:REF;/OPT:ICF
    )
    set(CAST_DEBUG_COMPILE_FLAGS
/wd4456;/wd4100;/wd4244;/permissive-;/Zc:rvalueCast;/W4;/FC;/Od;/Zi
    )
endif()

set(CAST_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE")

set(CAST_JUCE_VERSION 8.0.14)

find_package(Git REQUIRED)

message(STATUS "CAST: using JUCE at ${CAST_JUCE_PATH}")

add_subdirectory("${CAST_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")

get_directory_property(CAST_FOUND_JUCE_VERSION DIRECTORY "${CAST_JUCE_PATH}" DEFINITION JUCE_VERSION)

if(NOT CAST_FOUND_JUCE_VERSION VERSION_EQUAL CAST_JUCE_VERSION)
    message(FATAL_ERROR "CAST: JUCE ${CAST_FOUND_JUCE_VERSION} found, ${CAST_JUCE_VERSION} required — exact match")
endif()

message(STATUS "CAST: JUCE ${CAST_FOUND_JUCE_VERSION} found at ${CAST_JUCE_PATH}")

juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_core")      # JAM Core
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_subprocess")# JUCE-based subprocess launcher with streaming output and byte-cap truncation
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_markdown")  # Clean-room native CommonMark + GFM markdown parsing/rendering

message(STATUS "CAST: user modules added from ${CMAKE_CURRENT_SOURCE_DIR}/../jam")

juce_add_console_app(${PROJECT_NAME}
    PRODUCT_NAME "cast"
    VERSION "0.1.0"
    BUILD_VERSION "0.1.0"
    BUNDLE_ID "com.jrengmusic.cast"
    COMPANY_NAME "JRENG"
    COMPANY_COPYRIGHT "© 2026 JRENG"
    COMPANY_WEBSITE "https://jrengmusic.com"
    COMPANY_EMAIL "info@jrengmusic.com"
    DOCUMENT_EXTENSIONS cast
    NEEDS_CURL OFF
    NEEDS_WEB_BROWSER OFF
    NEEDS_WEBVIEW2 OFF
    NEEDS_STORE_KIT OFF
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
${CMAKE_CURRENT_SOURCE_DIR}/../jam
${CMAKE_CURRENT_SOURCE_DIR}/Source/generated
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
juce_core
juce_gui_basics
jam_core
jam_subprocess
jam_markdown
    PUBLIC
        juce::juce_recommended_config_flags
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:${CAST_RELEASE_COMPILE_FLAGS}>
    $<$<CONFIG:Debug>:${CAST_DEBUG_COMPILE_FLAGS}>
)

target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:${CAST_RELEASE_LINK_FLAGS}>
)

file(GLOB CAST_BINARY_FILES CONFIGURE_DEPENDS
Source/HELP.md
Source/resources/cast-output.md
)

juce_add_binary_data(${PROJECT_NAME}_BinaryData
    NAMESPACE "BinaryData"
    SOURCES
        ${CAST_BINARY_FILES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_BinaryData)

include(CheckIPOSupported)

add_custom_target(post-build ALL)
add_dependencies(post-build ${PROJECT_NAME})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    check_ipo_supported(RESULT CAST_IPO_SUPPORTED OUTPUT CAST_IPO_ERROR)

    if(CAST_IPO_SUPPORTED)
        set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
        message(STATUS "CAST: LTO enabled")
    else()
        message(WARNING "CAST: LTO not supported — ${CAST_IPO_ERROR}")
    endif()

    if(CAST_SIGN AND APPLE)
        string(TIMESTAMP CAST_BUILD_DATE "%Y-%m-%d" UTC)
        set(CAST_QA_PATH "$ENV{HOME}/Documents/Poems/dev/___builds___")
        set(CAST_QA_DIRECTORY "${CAST_QA_PATH}/${CAST_BUILD_DATE} ${PROJECT_NAME} v${PROJECT_VERSION} macOS")
        set(CAST_INSTALL_ROOT /opt)
        file(WRITE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_pkg_scripts/postinstall"
"#!/bin/bash
REAL_USER=$(stat -f \"%Su\" /dev/console)
REAL_HOME=$(eval echo ~$REAL_USER)
mkdir -p \"$REAL_HOME/.local/bin\"
ln -sf \"${CAST_INSTALL_ROOT}/${PROJECT_NAME}/${PROJECT_NAME}\" \"$REAL_HOME/.local/bin/${PROJECT_NAME}\"
")
        file(CHMOD "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_pkg_scripts/postinstall" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
        add_custom_command(TARGET post-build POST_BUILD
COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${PROJECT_NAME}>
COMMAND xattr -cr $<TARGET_FILE:${PROJECT_NAME}>
COMMAND codesign --force --options runtime --entitlements "${CMAKE_SOURCE_DIR}/entitlements.plist" --sign "Developer ID Application: Bayu Ardianto \\(9BDSN9TDX3\\)" $<TARGET_FILE:${PROJECT_NAME}>
COMMAND codesign --verify --verbose $<TARGET_FILE:${PROJECT_NAME}>
COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_pkg_root${CAST_INSTALL_ROOT}/${PROJECT_NAME}"
COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_pkg_root${CAST_INSTALL_ROOT}/${PROJECT_NAME}/"
COMMAND pkgbuild --root "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_pkg_root${CAST_INSTALL_ROOT}" --install-location "${CAST_INSTALL_ROOT}" --scripts "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_pkg_scripts" --identifier "com.jrengmusic.cast" --version "${PROJECT_VERSION}" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pkg"
COMMAND productsign --sign "Developer ID Installer: Bayu Ardianto \\(9BDSN9TDX3\\)" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pkg" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pkg.signed"
COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pkg.signed" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pkg"
COMMAND xcrun notarytool submit "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pkg" --keychain-profile "notary" --wait
COMMAND xcrun stapler staple "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pkg"
COMMAND ${CMAKE_COMMAND} -E make_directory "${CAST_QA_DIRECTORY}"
COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pkg" "${CAST_QA_DIRECTORY}/${PROJECT_NAME}.pkg"
            COMMENT "CAST: sign"
        )
    endif()
endif()

add_custom_command(TARGET post-build POST_BUILD
COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/.local/bin"
COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "$ENV{HOME}/.local/bin/${PROJECT_NAME}.tmp"
COMMAND ${CMAKE_COMMAND} -E rename "$ENV{HOME}/.local/bin/${PROJECT_NAME}.tmp" "$ENV{HOME}/.local/bin/${PROJECT_NAME}"
COMMAND ${CMAKE_COMMAND} -DPROJECT_ROOT=${CMAKE_SOURCE_DIR} -DBUILD_DIR=${CMAKE_BINARY_DIR} -DJAM_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/../jam -DTARGET_NAME=${PROJECT_NAME} -DAPPLE=${APPLE} -DWIN32=${WIN32} -P ${CMAKE_CURRENT_SOURCE_DIR}/../jam/cmake/ClangdConfig.cmake
    COMMENT "CAST: post-build"
)

message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "CAST: ${PROJECT_NAME} configured")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
