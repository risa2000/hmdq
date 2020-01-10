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

cmake_minimum_required (VERSION 3.15)

include(utils)

# Project def
# ============
project (hmdv
    VERSION ${GIT_REPO_VERSION_MAJOR}.${GIT_REPO_VERSION_MINOR}.${GIT_REPO_VERSION_PATCH}
    DESCRIPTION "displaying hmdq output data in no time"
    HOMEPAGE_URL "https://github.com/risa2000/hmdq"
    )

# Dependencies
# ============
find_package (xtensor 0.20.8 REQUIRED)
find_package (nlohmann_json 3.6.1 REQUIRED)
find_package (fmt 5.3.0 REQUIRED)

# Project defines
# ============
set (PROJECT_RES_DIR ${PROJECT_SOURCE_DIR}/res)

# Custom files
# ============
configure_file (
    ${PROJECT_RES_DIR}/misc.h.in
    ${PROJECT_BINARY_DIR}/misc.h
    )

# add output dir to the search path so "misc.h" is reachable
include_directories (${PROJECT_BINARY_DIR})

# copy the OpenVR API json file so the exe can run directly from there
configure_file (
    ${PROJECT_SOURCE_DIR}/../api/openvr_api.json
    ${PROJECT_BINARY_DIR}/openvr_api.json
    COPYONLY
    )

# needed for version info
if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set (VI_DEBUG ON)
endif()

# process version info and the icon
configure_file (
    ${PROJECT_RES_DIR}/hmdv.rc.in
    ${PROJECT_BINARY_DIR}/hmdv.rc
    )

# Targets
# ============
set (hmdq_dir ../hmdq)
set (hmdv_sources
    hmdv.cpp
    verhlp.cpp
    hmdfix.cpp
    ${hmdq_dir}/config.cpp
    ${hmdq_dir}/geom.cpp
    ${hmdq_dir}/hmddef.cpp
    ${hmdq_dir}/hmdview.cpp
    ${hmdq_dir}/jtools.cpp
    ${hmdq_dir}/optmesh.cpp
    ${hmdq_dir}/prtdata.cpp
    ${hmdq_dir}/wintools.cpp
    ${hmdq_dir}/xtdef.cpp
    ${PROJECT_BINARY_DIR}/hmdv.rc
    )

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:inline")
link_directories(AFTER ${CMAKE_INSTALL_PREFIX}/lib)

# Add sources to `hmdv` executable.
add_executable (hmdv)
target_sources (hmdv PRIVATE ${hmdv_sources})
target_include_directories (hmdv PRIVATE ${hmdq_dir})
target_include_directories (hmdv PRIVATE ${CMAKE_INSTALL_PREFIX}/include/botan-2)
target_link_libraries (hmdv PRIVATE version.lib
    debug botand.lib
    optimized botan.lib
    )
target_link_libraries (hmdv PRIVATE fmt::fmt)
#target_link_libraries (hmdv PRIVATE fmt::fmt-header-only)

# Generate a binary dump for a quick check of a CRT linakge type
add_custom_command (TARGET hmdv
    POST_BUILD
    COMMAND dumpbin /headers /symbols /imports "$<TARGET_FILE:hmdv>" > "$<TARGET_FILE:hmdv>.txt"
    )

# Install
# ============
install (FILES 
    ${CMAKE_SOURCE_DIR}/docs/images/hmdv_128.png
    DESTINATION ./docs/images
    )

install (TARGETS hmdv
    DESTINATION .
    )

print_variables ("hmdv_.*")
