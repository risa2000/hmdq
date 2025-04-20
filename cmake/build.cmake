#----------------------------------------------------------------------------+
# HMDQ Tools - tools for VR headsets and other hardware introspection        |
# https://github.com/risa2000/hmdq                                           |
#                                                                            |
# Copyright (c) 2025, Richard Musil. All rights reserved.                    |
#                                                                            |
# This source code is licensed under the BSD 3-Clause "New" or "Revised"     |
# License found in the LICENSE file in the root directory of this project.   |
# SPDX-License-Identifier: BSD-3-Clause                                      |
#----------------------------------------------------------------------------+

string (TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")
message (STATUS "BUILD_TIMESTAMP = ${BUILD_TIMESTAMP}")

string (TIMESTAMP BUILD_YEAR "%Y")
message (STATUS "BUILD_YEAR = ${BUILD_YEAR}")

