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

# Dummy project root
project (hmdq_root)

# Add path for modules (helpers)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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

# Include sub-projects.
add_subdirectory ("hmdq")

if (BUILD_TESTS)
    add_subdirectory ("hmdq_test")
endif (BUILD_TESTS)
