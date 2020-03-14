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

#include "base_common.h"

namespace basevr {

//  generic functions
//------------------------------------------------------------------------------
//  Resolve property tag enum from the type name.
PropType ptype_from_ptypename(const std::string& ptype_name)
{
    if (ptype_name == "Float") {
        return PropType::Float;
    }
    else if (ptype_name == "Int32") {
        return PropType::Int32;
    }
    else if (ptype_name == "Uint64") {
        return PropType::Uint64;
    }
    else if (ptype_name == "Bool") {
        return PropType::Bool;
    }
    else if (ptype_name == "String") {
        return PropType::String;
    }
    else if (ptype_name == "Matrix34") {
        return PropType::Matrix34;
    }
    else if (ptype_name == "Matrix44") {
        return PropType::Matrix44;
    }
    else if (ptype_name == "Vector2") {
        return PropType::Vector2;
    }
    else if (ptype_name == "Vector3") {
        return PropType::Vector3;
    }
    else if (ptype_name == "Vector4") {
        return PropType::Vector4;
    }
    else if (ptype_name == "Quad") {
        return PropType::Quad;
    }
    else {
        return PropType::Invalid;
    }
}

//  Return {<str:base_name>, <str:type_name>, <enum:type>, <bool:array>}
std::tuple<std::string, std::string, PropType, bool>
parse_prop_name(const std::string& pname)
{
    bool is_array = false;
    // cutting out the prefix
    const auto lpos1 = pname.find('_');
    // property type (the last part after '_')
    auto rpos1 = pname.rfind('_');
    auto ptype = pname.substr(rpos1 + 1);
    if (ptype == "Array") {
        const auto rpos2 = pname.rfind('_', rpos1 - 1);
        ptype = pname.substr(rpos2 + 1, rpos1 - rpos2 - 1);
        rpos1 = rpos2;
        is_array = true;
    }
    const auto basename = pname.substr(lpos1 + 1, rpos1 - lpos1 - 1);
    const auto type_name = pname.substr(rpos1 + 1);
    return {basename, type_name, ptype_from_ptypename(ptype), is_array};
}

} // namespace basevr
