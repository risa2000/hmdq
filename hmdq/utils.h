/******************************************************************************
 * HMDQ - Query tool for an OpenVR HMD and some other hardware                *
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
#include <stdexcept>
#include <vector>

//  debugging functions
//------------------------------------------------------------------------------
// Write the standard type to std::cout prefixed by the name
template<typename T>
inline void dump_st(const char* name, const T& v)
{
    std::cout << name << " = " << v << '\n';
}

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

//  custom runtime exception
//------------------------------------------------------------------------------
class hmdq_exception : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

class hmdq_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

#define HMDQ_EXCEPTION(_msg)                                                             \
    {                                                                                    \
        auto _file = std::string(__FILE__);                                              \
        auto _line = std::to_string(__LINE__);                                           \
        auto _smsg = std::string(_msg);                                                  \
        auto _res = std::string("hmdq runtime error:") + std::string("\nfile: ") + _file \
            + std::string("\nline: ") + _line + std::string("\n") + _smsg;               \
        throw hmdq_exception(_res);                                                      \
    }

#define HMDQ_ASSERT(_expr)                                                               \
    {                                                                                    \
        if (!(_expr)) {                                                                  \
            HMDQ_EXCEPTION("assert: (" #_expr ")");                                      \
        }                                                                                \
    }

//  general stuff
//------------------------------------------------------------------------------
// positive modulo (result is alway >= 0)
inline long long mod_pos(long long op, long long mod)
{
    long long res = op % mod;
    return (res >= 0) ? res : res + mod;
}
