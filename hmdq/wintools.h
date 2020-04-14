/******************************************************************************
 * HMDQ Tools - tools for VR headsets and other hardware introspection        *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2019, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#pragma once

#include <filesystem>
#include <string>
#include <vector>

//  Print system error message for ::GetLastError to stderr, add `message` as contextual
//  info.
bool print_sys_error(const char* message);

//  Get OS version or "n/a" if the attempt fails (print error in DEBUG build)
std::string get_os_ver();

//  Init console output code page
void init_console_cp();

//  Set console output code page, if installed
void set_console_cp(unsigned int codepage);

//  Return command line arguments as UTF-8 string list (in vector).
std::vector<std::string> get_u8args();

//  Get C-like args array from the list of strings (in a vector).
std::tuple<std::shared_ptr<std::vector<char*>>, std::shared_ptr<std::vector<char>>>
get_c_argv(const std::vector<std::string>& args);

//  Return the full path of the executable which created the process
std::filesystem::path get_full_prog_path();

//  Print command line arguments (for debugging purposes)
void print_u8args(std::vector<std::string> u8args);
