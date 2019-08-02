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
# Set policy to accept MSVC runtime selector (requires CMake >= 3.15)
cmake_policy (SET CMP0091 NEW)

# Dummy project root
project (hmdq_root)

# Add path for modules (helpers)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include sub-projects.
add_subdirectory ("hmdq")
