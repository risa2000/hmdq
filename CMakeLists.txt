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
# Set policy to accept MSVC runtime selector (requires CMake >= 3.15)
cmake_policy (SET CMP0091 NEW)
 
# Add path for modules (helpers)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Versioning
# ============
include(gitversion)
message (STATUS "GIT_REPO_VERSION = ${GIT_REPO_VERSION}")

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

# Project root
# ============
project (hmdq_tools
    VERSION ${GIT_REPO_VERSION_MAJOR}.${GIT_REPO_VERSION_MINOR}.${GIT_REPO_VERSION_PATCH}
    DESCRIPTION "Tools for OpenVR HMD and other hardware introspection."
    HOMEPAGE_URL "https://github.com/risa2000/hmdq"
    )

# Include sub-projects.
add_subdirectory ("hmdq")
add_subdirectory ("hmdv")

if (BUILD_TESTS)
    add_subdirectory ("hmdq_test")
endif (BUILD_TESTS)

# Install
# ============
install (FILES 
    ${CMAKE_SOURCE_DIR}/api/openvr_api.json
    ${CMAKE_SOURCE_DIR}/README.md
    ${CMAKE_SOURCE_DIR}/LICENSE
    DESTINATION .
    )

set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set (CPACK_PACKAGE_VENDOR $ENV{USERNAME})
set (CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set (CPACK_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "hmdq tools package")
set (CPACK_PACKAGE_HOMEPAGE_URL ${PROJECT_HOMEPAGE_URL})
set (CPACK_GENERATOR "ZIP" CACHE STRING "Generators to support. semi-colon delimited list")
include(CPack)
