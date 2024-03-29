﻿#----------------------------------------------------------------------------+
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
project (hmdq_test
    DESCRIPTION "Unit tests for hmdq tool"
    )

# Dependencies
# ============
find_package (fmt REQUIRED)
find_package (xtensor REQUIRED)
find_package (Eigen3 REQUIRED)
# Use Catch2 unit test framework
find_package (Catch2 REQUIRED)

set (hmdq_dir ../hmdq)
set (hmdv_dir ../hmdv)

set (hmdq_test_sources
    ${hmdq_dir}/geom.cpp
    ${hmdq_dir}/geom2.cpp
    geom_test.cpp # this one defines main
    ${hmdq_dir}/optmesh.cpp
    optmesh_test.cpp
    ${hmdq_dir}/xtdef.cpp
    ${hmdv_dir}/verhlp.cpp
    verhlp_test.cpp
    )

# Add unity tests
add_executable (hmdq_test)
target_sources (hmdq_test PRIVATE ${hmdq_test_sources})

target_include_directories (hmdq_test PRIVATE ${hmdq_dir} ${hmdv_dir})

target_compile_options(hmdq_test PRIVATE /Zc:inline /Zc:__cplusplus)

# turn on linker optimization back on in RelWithDebInfo config
target_link_options (hmdq_test PRIVATE $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO /OPT:REF /OPT:ICF>)

target_link_libraries (hmdq_test PRIVATE fmt::fmt Catch2::Catch2WithMain Eigen3::Eigen xtensor)

enable_testing ()

add_test(NAME hmdq_test COMMAND hmdq_test)

print_variables ("hmdq_test*")
