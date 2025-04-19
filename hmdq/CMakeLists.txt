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

include(utils)

# Project def
# ============
project (hmdq
    VERSION ${GIT_REPO_VERSION_MAJOR}.${GIT_REPO_VERSION_MINOR}.${GIT_REPO_VERSION_PATCH}
    DESCRIPTION "collecting VR headsets characteristics in no time"
    HOMEPAGE_URL "https://github.com/risa2000/hmdq"
    )

# Dependencies
# ============
find_package (clipp REQUIRED)
find_package (fmt REQUIRED)
find_package (nlohmann_json REQUIRED)
find_package (xtensor REQUIRED)
find_package (Eigen3 REQUIRED)
find_package (openvr REQUIRED)
find_package (botan REQUIRED)
find_package (geos REQUIRED)

# Project defines
# ============
set (PROJECT_RES_DIR ${PROJECT_SOURCE_DIR}/res)

# Custom files
# ============
configure_file (
    ${CMAKE_SOURCE_DIR}/res/misc.h.in
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

# copy the batch file to save the hmdq data into .json file
configure_file (
    ${PROJECT_SOURCE_DIR}/../res/save_data.cmd
    ${PROJECT_BINARY_DIR}/save_data.cmd
    COPYONLY
    )

# needed for version info
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set (VI_DEBUG ON)
endif()

# process version info and the icon
configure_file (
    ${PROJECT_RES_DIR}/hmdq.rc.in
    ${PROJECT_BINARY_DIR}/hmdq.rc
    )

# Targets
# ============
set (hmdq_SOURCES
    calcview.cpp
    config.cpp
    geom.cpp
    geom2.cpp
    hmdq.cpp
    jkeys.cpp
    jtools.cpp
    base_common.cpp
    oculus_common.cpp
    oculus_config.cpp
    oculus_collector.cpp
    oculus_processor.cpp
    openvr_common.cpp
    openvr_config.cpp
    openvr_collector.cpp
    openvr_processor.cpp
    optmesh.cpp
    prtdata.cpp
    wintools.cpp
    xtdef.cpp
    ${PROJECT_BINARY_DIR}/hmdq.rc
    )

# 3rd party libs (linked as .lib, but needed as .dll in DLL CRT linkage)
set (EXT_LIBS botan openvr_api64)

# Add sources to `hmdq` executable.
add_executable (hmdq)
target_sources (hmdq PRIVATE ${hmdq_SOURCES})

target_link_libraries (hmdq PRIVATE build_proxy)
target_link_libraries (hmdq PRIVATE version.lib)
target_link_libraries (hmdq PRIVATE
    clipp::clipp
    fmt::fmt
    nlohmann_json::nlohmann_json
    xtensor
    Eigen3::Eigen
    openvr::openvr
    botan::botan
    GEOS::geos
)

if (OVR_SDK)
    target_link_libraries (hmdq PRIVATE LibOVR)
endif()

# Generate a binary dump for a quick check of a CRT linkage type
add_custom_command (TARGET hmdq
    POST_BUILD
    COMMAND dumpbin /headers /symbols /imports "$<TARGET_FILE:hmdq>" > "$<TARGET_FILE:hmdq>.txt"
    )

# Check the CRT linkage, add DLLs if needed
#if (${CMAKE_MSVC_RUNTIME_LIBRARY} MATCHES "^.*DLL>?$" OR NOT CMAKE_MSVC_RUNTIME_LIBRARY)
#    collect_dlls(hmdq ${EXT_LIBS})
#endif()

# Install
# ============
install (FILES 
    ${CMAKE_SOURCE_DIR}/docs/images/hmdq_128.png
    DESTINATION ./docs/images
    )

install (TARGETS hmdq
    DESTINATION .
    )

print_variables ("eigen3_.*")
