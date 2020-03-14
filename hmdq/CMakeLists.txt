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
project (hmdq
    VERSION ${GIT_REPO_VERSION_MAJOR}.${GIT_REPO_VERSION_MINOR}.${GIT_REPO_VERSION_PATCH}
    DESCRIPTION "collecting OpenVR headset characteristics in no time"
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

# copy the batch file to save the hmdq data into .json file
configure_file (
    ${PROJECT_SOURCE_DIR}/../res/save_data.cmd
    ${PROJECT_BINARY_DIR}/save_data.cmd
    COPYONLY
    )

# needed for version info
if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set (VI_DEBUG ON)
endif()

# process version info and the icon
configure_file (
    ${PROJECT_RES_DIR}/hmdq.rc.in
    ${PROJECT_BINARY_DIR}/hmdq.rc
    )

# Targets
# ============
set (hmdq_sources
    calcview.cpp
    config.cpp
    geom.cpp
    hmdq.cpp
    jtools.cpp
    base_common.cpp
    oculus_collector.cpp
    oculus_config.cpp
    oculus_props.cpp
    openvr_collector.cpp
    openvr_common.cpp
    openvr_config.cpp
    openvr_processor.cpp
    optmesh.cpp
    prtdata.cpp
    wintools.cpp
    xtdef.cpp
    ${PROJECT_BINARY_DIR}/hmdq.rc
    )

# mangle the lib name for Clang build
if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    set (OPENVRLIB openvr_api64c)
else()
    set (OPENVRLIB openvr_api64)
endif()

# set the path to OVR_SDK
if (NOT OVR_SDK)
    set (OVR_SDK $ENV{OVR_SDK})
endif()

# 3rd party libs (linked as .lib, but needed as .dll in DLL CRT linkage)
set (EXT_LIBS botan ${OPENVRLIB})

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:inline")
link_directories(AFTER ${CMAKE_INSTALL_PREFIX}/lib)

# Add sources to `hmdq` executable.
add_executable (hmdq)
target_sources (hmdq PRIVATE ${hmdq_sources})
target_include_directories (hmdq PRIVATE ${CMAKE_INSTALL_PREFIX}/include/botan-2)
target_include_directories (hmdq PRIVATE ${OVR_SDK}/LibOVR/Include)
target_link_directories (hmdq PRIVATE ${OVR_SDK}/LibOVR/Lib/Windows/x64/$<IF:$<CONFIG:Debug>,Debug,Release>)
target_link_libraries (hmdq PRIVATE version.lib)
target_link_libraries (hmdq PRIVATE LibOVR.lib)
foreach (lib IN ITEMS ${EXT_LIBS})
    target_link_libraries (hmdq PRIVATE debug ${lib}d.lib)
    target_link_libraries (hmdq PRIVATE optimized ${lib}.lib)
endforeach()
target_link_libraries (hmdq PRIVATE fmt::fmt)
#target_link_libraries (hmdq PRIVATE fmt::fmt-header-only)

# Generate a binary dump for a quick check of a CRT linakge type
add_custom_command (TARGET hmdq
    POST_BUILD
    COMMAND dumpbin /headers /symbols /imports "$<TARGET_FILE:hmdq>" > "$<TARGET_FILE:hmdq>.txt"
    )

# Check the CRT linkage, add DLLs if needed
if (${CMAKE_MSVC_RUNTIME_LIBRARY} MATCHES "^.*DLL$" OR NOT CMAKE_MSVC_RUNTIME_LIBRARY)
    collect_dlls(hmdq ${EXT_LIBS})
endif()

# Install
# ============
install (FILES 
    ${CMAKE_SOURCE_DIR}/docs/images/hmdq_128.png
    DESTINATION ./docs/images
    )

install (TARGETS hmdq
    DESTINATION .
    )

print_variables ("hmdq_.*")
