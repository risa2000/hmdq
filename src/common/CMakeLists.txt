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

# Targets
# ============
set (hmdq_common_SOURCES
    base_common.cpp
    calcview.cpp
    config.cpp
    geom.cpp
    geom2.cpp
    jtools.cpp
    jkeys.cpp
    openvr_common.cpp
    openvr_config.cpp
    openvr_processor.cpp
    oculus_common.cpp
    oculus_config.cpp
    oculus_processor.cpp
    optmesh.cpp
    prtdata.cpp
    verhlp.cpp
    wintools.cpp
    xtdef.cpp
    )

# Add sources to `hmdq` executable.
add_library (hmdq_common STATIC ${hmdq_common_SOURCES})
target_sources (hmdq_common PRIVATE ${hmdq_SOURCES})
target_link_libraries (hmdq_common PUBLIC build_proxy)
target_link_libraries (hmdq_common PUBLIC
    clipp::clipp
    fmt::fmt
    nlohmann_json::nlohmann_json
    xtensor
    Eigen3::Eigen
    openvr::openvr
    botan::botan
    GEOS::geos
    LibOVR
)
