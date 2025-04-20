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

#include <fmt/format.h>

#include <cstdio>
#include <utility>

//  These functions are attributed to vitaut@github as per this discussion
//  https://github.com/fmtlib/fmt/issues/1260
template <typename... T>
void iprint(int indent, fmt::format_string<T...> format_str, T&&... args)
{
    fmt::print("{:{}}", "", indent);
    fmt::print(format_str, std::forward<T>(args)...);
}

template <typename... T>
void iprint(std::FILE* f, int indent, fmt::format_string<T...> format_str, T&&... args)
{
    fmt::print(f, "{:{}}", "", indent);
    fmt::print(f, format_str, std::forward<T>(args)...);
}
