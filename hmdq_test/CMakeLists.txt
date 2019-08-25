﻿#----------------------------------------------------------------------------+
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
project (hmdq_test
    DESCRIPTION "Unit tests for hmdq tool"
    )

# Dependencies
# ============
find_package (xtensor 0.20.8 REQUIRED)
# Use Catch2 unit test framework
find_package (Catch2 REQUIRED)

set (hmdq_dir ../hmdq)
set (hmdq_test_sources
    ${hmdq_dir}/geom.cpp
    geom_test.cpp # this one defines main
    ${hmdq_dir}/optmesh.cpp
    optmesh_test.cpp
    ${hmdq_dir}/xtdef.cpp
    )

# Add unity tests
add_executable (hmdq_test)
target_sources (hmdq_test PRIVATE ${hmdq_test_sources})
target_include_directories (hmdq_test PRIVATE ${hmdq_dir})
target_link_libraries (hmdq_test PRIVATE Catch2::Catch2)
enable_testing ()
add_test(NAME hmdq_test COMMAND hmdq_test)

print_variables ("hmdq_test*")