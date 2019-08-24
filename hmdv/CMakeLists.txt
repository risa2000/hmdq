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
    "${PROJECT_RES_DIR}/hmdv.rc.in"
    "${PROJECT_BINARY_DIR}/hmdv.rc"
    )

# Targets
# ============
set (hmdq_dir ../hmdq)
set (hmdv_sources
    hmdv.cpp
    ${hmdq_dir}/config.cpp
    #${hmdq_dir}/geom.cpp
    #${hmdq_dir}/hmdview.cpp
    ${hmdq_dir}/jtools.cpp
    #${hmdq_dir}/optmesh.cpp
    ${hmdq_dir}/wintools.cpp
    #${hmdq_dir}/ovrhlp.cpp
    ${hmdq_dir}/prtdata.cpp
    ${hmdq_dir}/xtdef.cpp
    ${PROJECT_BINARY_DIR}/hmdv.rc
    )

#if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
#    set (OPENVRLIB openvr_api64c)
#else()
#    set (OPENVRLIB openvr_api64)
#endif()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:inline")
link_directories(AFTER "${CMAKE_PREFIX_PATH}/lib")

# Add sources to `hmdv` executable.
add_executable (hmdv)
#target_compile_options (hmdv PRIVATE /Gw)
#target_link_options (hmdv PRIVATE /OPT:REF)
target_sources (hmdv PRIVATE ${hmdv_sources})
target_include_directories (hmdv PRIVATE ${hmdq_dir})
target_include_directories (hmdv PRIVATE "${CMAKE_PREFIX_PATH}/include")
target_include_directories (hmdv PRIVATE "${CMAKE_PREFIX_PATH}/include/botan-2")
target_link_libraries (hmdv PRIVATE version.lib
    debug botand.lib
    optimized botan.lib
    #debug ${OPENVRLIB}d debug botand.lib
    #optimized ${OPENVRLIB} optimized botan.lib
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

install (FILES 
    ${CMAKE_SOURCE_DIR}/api/openvr_api.json
    ${CMAKE_SOURCE_DIR}/README.md
    ${CMAKE_SOURCE_DIR}/LICENSE
    DESTINATION .
    )

install (TARGETS hmdv
    DESTINATION .
    )

set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set (CPACK_PACKAGE_NAME ${PROJECT_NAME})
set (CPACK_PACKAGE_VENDOR $ENV{USERNAME})
set (CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set (CPACK_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "hmdv package")
set (CPACK_PACKAGE_HOMEPAGE_URL ${PROJECT_HOMEPAGE_URL})
set (CPACK_GENERATOR "ZIP" CACHE STRING "Generators to support. semi-colon delimited list")
include(CPack)

print_variables ("hmdv_.*")
