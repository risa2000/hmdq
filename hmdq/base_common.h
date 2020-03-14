/******************************************************************************
 * HMDQ Tools - tools for an OpenVR HMD and other hardware introspection      *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2020, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#pragma once

#include <string>
#include <tuple>

#include "fifo_map_fix.h"

namespace basevr {

//  property types
//------------------------------------------------------------------------------
// Use these enums to define common types used in VR subsystems
enum class PropType : int {
    Invalid = -1,

    Float = 1,
    Double = 2,
    Int32 = 3,
    Uint64 = 4,
    Bool = 5,
    String = 6,

    Vector2 = 12,
    Vector3 = 13,
    Vector4 = 14,

    Matrix33 = 33,
    Matrix34 = 34,
    Matrix44 = 44,

    Quad = 50,
    Quaternion = 60,
};

//  Generic functions.
//------------------------------------------------------------------------------
//  Return {<str:base_name>, <str:type_name>, <enum:type>, <bool:array>}
std::tuple<std::string, std::string, PropType, bool>
parse_prop_name(const std::string& pname);

//  Print functions.
//------------------------------------------------------------------------------
//  Print one property out (do not print PID < 0)
void print_one_prop(const std::string& pname, const json& pval, int pid,
                    const json& verb_props, int verb, int ind, int ts);

} // namespace basevr
