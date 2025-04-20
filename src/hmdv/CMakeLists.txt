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
project (hmdv
    VERSION ${GIT_REPO_VERSION_MAJOR}.${GIT_REPO_VERSION_MINOR}.${GIT_REPO_VERSION_PATCH}
    DESCRIPTION "displaying hmdq output data in no time"
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

# Custom files
# ============
# copy the OpenVR API json file so the exe can run directly from there
configure_file (
    ${CMAKE_SOURCE_DIR}/api/openvr_api.json
    ${CMAKE_CURRENT_BINARY_DIR}/openvr_api.json
    COPYONLY
    )

# needed for version info
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set (VI_DEBUG ON)
endif()

# process version info and the icon
configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/res/hmdv.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/hmdv.rc
    )

# Targets
# ============
set (hmdv_SOURCES
    hmdv.cpp
    hmdfix.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/hmdv.rc
    )

# 3rd party libs (linked as .lib, but needed as .dll in DLL CRT linkage)
set (EXT_LIBS botan)

# Add sources to `hmdv` executable.
add_executable (hmdv)
target_sources (hmdv PRIVATE ${hmdv_SOURCES})

target_link_libraries (hmdv PRIVATE build_proxy hmdq_common)
target_link_libraries (hmdv PRIVATE version.lib)
target_link_libraries (hmdv PRIVATE
    clipp::clipp
    fmt::fmt
    nlohmann_json::nlohmann_json
    xtensor
    Eigen3::Eigen
    botan::botan
    openvr::openvr
    GEOS::geos
    )

# Generate a binary dump for a quick check of a CRT linkage type
add_custom_command (TARGET hmdv
    POST_BUILD
    COMMAND dumpbin /headers /symbols /imports "$<TARGET_FILE:hmdv>" > "$<TARGET_FILE:hmdv>.txt"
    )

# Check the CRT linkage, add DLLs if needed
#if (${CMAKE_MSVC_RUNTIME_LIBRARY} MATCHES "^.*DLL>?$" OR NOT CMAKE_MSVC_RUNTIME_LIBRARY)
#    collect_dlls(hmdv ${EXT_LIBS})
#endif()

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
