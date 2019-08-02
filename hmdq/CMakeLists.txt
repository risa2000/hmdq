#----------------------------------------------------------------------------+
# HMDQ - Query tool for an OpenVR HMD and some other hardware                |
# https://github.com/risa2000/hmdq                                           |
#                                                                            |
# Copyright (c) 2019, Richard Musil. All rights reserved.                    |
#                                                                            |
# This source code is licensed under the BSD 3-Clause "New" or "Revised"     |
# License found in the LICENSE file in the root directory of this project.   |
# SPDX-License-Identifier: BSD-3-Clause                                      |
#----------------------------------------------------------------------------+

cmake_minimum_required (VERSION 3.15)

include(utils)

# Versioning
# ============
include(gitversion)
message (STATUS "GIT_REPO_VERSION = ${GIT_REPO_VERSION}")

# Project def
# ============
project (hmdq
    VERSION ${GIT_REPO_VERSION_MAJOR}.${GIT_REPO_VERSION_MINOR}.${GIT_REPO_VERSION_PATCH}
    DESCRIPTION "collecting OpenVR headset characteristics in no time"
    HOMEPAGE_URL "https://github.com/risa2000/hmdq"
    )

# Dependencies
# ============
find_package (xtensor 0.20.8 REQUIRED)
find_package (nlohmann_json 3.6.1 REQUIRED)

# Standards
# ============
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# CMake options
# ============
option(BUILD_TESTS "Build optional unit tests (needs Catch2 lib)" ON)
option(BUILD_STATIC "Build the executables with statically linked runtime libraries" ON)
if (BUILD_STATIC)
    set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    message (STATUS "Building with static CRT.")
endif()

# Project defines
# ============
set (PROJECT_RES_DIR ${PROJECT_SOURCE_DIR}/res)

# Custom files
# ============
configure_file (
    "${PROJECT_RES_DIR}/misc.h.in"
    "${PROJECT_BINARY_DIR}/misc.h"
    )

# add output dir to the search path so "misc.h" is reachable
include_directories (${PROJECT_BINARY_DIR})

# copy the OpenVR API json file so the exe can run directly from there
configure_file (
    "${PROJECT_SOURCE_DIR}/../api/openvr_api.json"
    "${PROJECT_BINARY_DIR/openvr_api.json}"
    COPYONLY
    )

# needed for version info
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set (VI_DEBUG ON)
endif()

# process version info and the icon
configure_file (
    "${PROJECT_RES_DIR}/hmdq.rc.in"
    "${PROJECT_BINARY_DIR}/hmdq.rc"
    )

# Targets
# ============
set (hmdq_sources
    hmdq.cpp
    config.cpp
    geom.cpp
    hmdview.cpp
    optmesh.cpp
    osver.cpp
    ovrhlp.cpp
    xtdef.cpp
    ${PROJECT_BINARY_DIR}/hmdq.rc
    )

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set (OPENVRLIB openvr_api64c)
else()
    set (OPENVRLIB openvr_api64)
endif()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:inline")
link_directories(AFTER "${CMAKE_PREFIX_PATH}/lib")

# Add sources to `hmdq` executable.
add_executable (hmdq)
target_sources (hmdq PRIVATE ${hmdq_sources})
target_include_directories (hmdq PRIVATE "${CMAKE_PREFIX_PATH}/include")
target_link_libraries (hmdq PRIVATE version.lib debug "${OPENVRLIB}d" optimized "${OPENVRLIB}")

# Generate a binary dump for a quick check of a CRT linakge type
add_custom_command (TARGET hmdq
    POST_BUILD
    COMMAND dumpbin /headers /symbols /imports "$<TARGET_FILE:hmdq>" > "$<TARGET_FILE:hmdq>.txt"
    )

# Tests
# ============
if (BUILD_TESTS)
    # Use Catch2 unit test framework
    find_package (Catch2 REQUIRED)

    set (hmdq_test_sources
        geom.cpp
        geom_test.cpp # this one defines main
        optmesh.cpp
        optmesh_test.cpp
        xtdef.cpp
        )

    # Add unity tests
    add_executable (hmdq_test)
    target_sources (hmdq_test PRIVATE ${hmdq_test_sources})
    target_link_libraries (hmdq_test Catch2::Catch2)
    enable_testing ()
    add_test(NAME hmdq_test COMMAND hmdq_test)
endif (BUILD_TESTS)

# Install
# ============
install (FILES 
    ${CMAKE_SOURCE_DIR}/docs/images/hmdq_128.png
    DESTINATION ./docs/images
    )

install (FILES 
    ${CMAKE_SOURCE_DIR}/api/openvr_api.json
    ${CMAKE_SOURCE_DIR}/README.md
    ${CMAKE_SOURCE_DIR}/LICENSE
    DESTINATION .
    )

install (TARGETS hmdq
    DESTINATION .
    )

set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set (CPACK_PACKAGE_NAME ${PROJECT_NAME})
set (CPACK_PACKAGE_VENDOR $ENV{USERNAME})
set (CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set (CPACK_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "hmdq package")
set (CPACK_PACKAGE_HOMEPAGE_URL ${PROJECT_HOMEPAGE_URL})
set (CPACK_GENERATOR "ZIP" CACHE STRING "Generators to support. semi-colon delimited list")
include(CPack)

print_variables ("hmdq_.*")
