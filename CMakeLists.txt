#----------------------------------------------------------------------------+
# HMDQ Tools - tools for VR headsets and other hardware introspection        |
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
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# CMake options
# ============
option(BUILD_TESTS "Build optional unit tests (needs Catch2 lib)" ON)

# Project root
# ============
project (hmdq_tools
    VERSION ${GIT_REPO_VERSION_MAJOR}.${GIT_REPO_VERSION_MINOR}.${GIT_REPO_VERSION_PATCH}
    DESCRIPTION "Tools for VR headset and other hardware introspection."
    HOMEPAGE_URL "https://github.com/risa2000/hmdq"
    )

# Global build defs (proxy target)
# ================================
add_library (build_proxy INTERFACE)
target_compile_options (build_proxy INTERFACE /Zc:inline /Zc:__cplusplus)
# turn on linker optimization back on in RelWithDebInfo config
target_link_options (build_proxy INTERFACE $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO /OPT:REF /OPT:ICF>)

# LibOVR stub (proxy target)
# ==========================
# set the path to OVR_SDK
if (NOT OVR_SDK)
    set (OVR_SDK $ENV{OVR_SDK})
endif()

if (OVR_SDK)
    add_library (LibOVR INTERFACE)
    target_compile_definitions (LibOVR INTERFACE OVR_SDK=${OVR_SDK})
    target_include_directories (LibOVR INTERFACE ${OVR_SDK}/LibOVR/Include)
    target_link_directories (LibOVR INTERFACE ${OVR_SDK}/LibOVR/Lib/Windows/x64/Release/VS2017)
    target_link_libraries (LibOVR INTERFACE LibOVR.lib)
endif()

# Include sub-projects
# ====================
add_subdirectory ("hmdq")
add_subdirectory ("hmdv")

if (BUILD_TESTS)
    include (CTest)
    include (Catch)

    add_subdirectory ("hmdq_test")
endif (BUILD_TESTS)

# Install
# ============
install (FILES 
    ${CMAKE_SOURCE_DIR}/api/openvr_api.json
    ${CMAKE_SOURCE_DIR}/res/save_data.cmd
    ${CMAKE_SOURCE_DIR}/README.md
    ${CMAKE_SOURCE_DIR}/Changelog.md
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
