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

#include <fmt/format.h>

//  These functions are attributed to vitaut@github as per this discussion
//  https://github.com/fmtlib/fmt/issues/1260
template<typename... Args>
void iprint(int indent, fmt::string_view format_str, Args&&... args)
{
    fmt::print("{:{}}", "", indent);
    fmt::print(format_str, std::forward<Args>(args)...);
}

template<typename... Args>
void iprint(std::FILE* f, int indent, fmt::string_view format_str, Args&&... args)
{
    fmt::print(f, "{:{}}", "", indent);
    fmt::print(f, format_str, std::forward<Args>(args)...);
}
