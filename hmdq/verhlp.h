/******************************************************************************
 * HMDQ Tools - tools for an OpenVR HMD and other hardware introspection      *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2019, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <string>
#include <tuple>

//  globals
//------------------------------------------------------------------------------
constexpr const char* PROG_VER_FOV_FIX = "1.2.4";

//  functions
//------------------------------------------------------------------------------
//  Get the first number from the version string.
std::tuple<int, size_t> first_num(const std::string& vs, size_t pos);

// Compare two versions `va` and `vb`.
// Return:
//  -1 : va < vb
//   0 : va == vb
//   1 : va > vb
int comp_ver(const std::string& va, const std::string& vb);
