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

#pragma once

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <string>
#include <vector>

//  Get OS version or "n/a" if the attempt fails (print error in DEBUG build)
std::string get_os_ver();

//  Set console output code page, if installed
void set_console_cp(unsigned int codepage);

//  Return command line arguments as UTF-8 string list (in vector).
std::vector<std::string> get_u8args();

//  Get C-like args array from the list of strings (in a vector).
std::tuple<std::vector<size_t>, std::vector<char>>
get_c_argv(const std::vector<std::string>& args);
