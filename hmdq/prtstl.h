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
#include <iostream>
#include <vector>

//  template functions
//------------------------------------------------------------------------------
//  Write std::pair to stdout
template<typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, const std::pair<T1, T2>& input)
{
    os << "{" << input.first << ", " << input.second << "}";
    return os;
}

template<typename T>
//  Write std::vector to stdout
std::ostream& operator<<(std::ostream& os, const std::vector<T>& input)
{
    os << "{";
    bool more = false;
    for (auto const& i : input) {
        if (more)
            os << ", ";
        else
            more = true;
        os << i;
    }
    os << "}";
    return os;
}
