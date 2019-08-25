#----------------------------------------------------------------------------+
# HMDQ Tools - tools for an OpenVR HMD and other hardware introspection      |
# https://github.com/risa2000/hmdq                                           |
#                                                                            |
# Copyright (c) 2019, Richard Musil. All rights reserved.                    |
#                                                                            |
# This source code is licensed under the BSD 3-Clause "New" or "Revised"     |
# License found in the LICENSE file in the root directory of this project.   |
# SPDX-License-Identifier: BSD-3-Clause                                      |
#----------------------------------------------------------------------------+

cmake_minimum_required (VERSION 3.8)

include (utils)

message (STATUS "Resolving GIT Version")

find_package (Git)
if (GIT_FOUND)
    execute_process (
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --match "v*"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_REPO_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    message (STATUS "GIT tag: ${GIT_REPO_VERSION}")
else()
    set (GIT_REPO_VERSION "unknown")
    message (STATUS "GIT not found")
endif()

if (GIT_REPO_VERSION MATCHES "^v([0-9]+).([0-9]+).([0-9]+)($|(-([0-9]+)-([a-h0-9]+)))")
    set (GIT_REPO_VERSION_MAJOR ${CMAKE_MATCH_1})
    set (GIT_REPO_VERSION_MINOR ${CMAKE_MATCH_2})
    set (GIT_REPO_VERSION_PATCH ${CMAKE_MATCH_3})
    set (GIT_REPO_VERSION_NCOMM ${CMAKE_MATCH_6})
    set (GIT_REPO_VERSION_HASH ${CMAKE_MATCH_7})
    foreach (i RANGE ${CMAKE_MATCH_COUNT})
        message (STATUS "CMAKE_MATCH_${i} = ${CMAKE_MATCH_${i}}")
    endforeach()
    print_variables("GIT_.*")
else()
    message (STATUS "No valid GIT version found, unsetting the var")
    unset (GIT_REPO_VERSION)
endif()

string (TIMESTAMP _TIMESTAMP "%Y-%m-%d %H:%M:%S")
message (STATUS "${_TIMESTAMP}")

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/res/gitversion.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/res/gitversion.h
    @ONLY
    )

# add output dir to the search path so the other subprojects find it
include_directories (${CMAKE_CURRENT_BINARY_DIR}/res)
