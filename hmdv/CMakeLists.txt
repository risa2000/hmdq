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
find_package (xtensor 0.21.4 REQUIRED)
find_package (nlohmann_json 3.7.3 REQUIRED)
find_package (fmt 6.1.3 REQUIRED)

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
    ${hmdq_dir}/base_common.cpp
    ${hmdq_dir}/calcview.cpp
    ${hmdq_dir}/config.cpp
    ${hmdq_dir}/geom.cpp
    ${hmdq_dir}/jtools.cpp
    ${hmdq_dir}/jkeys.cpp
    ${hmdq_dir}/openvr_common.cpp
    ${hmdq_dir}/openvr_config.cpp
    ${hmdq_dir}/openvr_processor.cpp
    ${hmdq_dir}/oculus_common.cpp
    ${hmdq_dir}/oculus_config.cpp
    ${hmdq_dir}/oculus_processor.cpp
    ${hmdq_dir}/optmesh.cpp
    ${hmdq_dir}/prtdata.cpp
    ${hmdq_dir}/wintools.cpp
    ${hmdq_dir}/xtdef.cpp
    ${PROJECT_BINARY_DIR}/hmdv.rc
    )

# set the path to OVR_SDK
if (NOT OVR_SDK)
    set (OVR_SDK $ENV{OVR_SDK})
endif()

# 3rd party libs (linked as .lib, but needed as .dll in DLL CRT linkage)
set (EXT_LIBS botan)

# Add sources to `hmdv` executable.
add_executable (hmdv)
target_sources (hmdv PRIVATE ${hmdv_sources})

target_include_directories (hmdv PRIVATE ${hmdq_dir})
target_include_directories (hmdv PRIVATE ${CMAKE_INSTALL_PREFIX}/include/botan-2)
target_include_directories (hmdv PRIVATE ${OVR_SDK}/LibOVR/Include)
target_include_directories (hmdv PRIVATE ${CMAKE_INSTALL_PREFIX}/include)

target_compile_options (hmdv PRIVATE /Zc:inline /Zc:__cplusplus)

target_link_directories (hmdv PRIVATE ${CMAKE_INSTALL_PREFIX}/lib)

# turn on linker optimization back on in RelWithDebInfo config
target_link_options (hmdv PRIVATE $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO /OPT:REF /OPT:ICF>)

target_link_libraries (hmdv PRIVATE version.lib)
foreach (lib IN ITEMS ${EXT_LIBS})
    target_link_libraries (hmdv PRIVATE debug ${lib}d.lib)
    target_link_libraries (hmdv PRIVATE optimized ${lib}.lib)
endforeach()
# fix the problem with fmt which links to debug build in RelWithDebInfo config
target_link_libraries(hmdv PRIVATE fmt$<$<CONFIG:Debug>:d>.lib)
#target_link_libraries (hmdv PRIVATE fmt::fmt-header-only)

# Generate a binary dump for a quick check of a CRT linkage type
add_custom_command (TARGET hmdv
    POST_BUILD
    COMMAND dumpbin /headers /symbols /imports "$<TARGET_FILE:hmdv>" > "$<TARGET_FILE:hmdv>.txt"
    )

# Check the CRT linkage, add DLLs if needed
if (${CMAKE_MSVC_RUNTIME_LIBRARY} MATCHES "^.*DLL$" OR NOT CMAKE_MSVC_RUNTIME_LIBRARY)
    collect_dlls(hmdv ${EXT_LIBS})
endif()
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
