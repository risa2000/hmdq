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

#include <stdexcept>

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
