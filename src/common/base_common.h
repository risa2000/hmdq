/******************************************************************************
 * HMDQ Tools - tools for VR headsets and other hardware introspection        *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2020, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#pragma once

#include <common/json_proxy.h>

#include <string>
#include <tuple>

namespace basevr {

//  property types
//------------------------------------------------------------------------------
// Use these enums to define common types used in VR subsystems
enum class PropType : int {
    Invalid,

    Float,
    Double,

    Int16,
    Uint16,
    Int32,
    Uint32,
    Int64,
    Uint64,

    Bool,
    String,

    Vector2,
    Vector3,
    Vector4,

    Matrix33,
    Matrix34,
    Matrix44,

    Quaternion,

    Quad,
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
