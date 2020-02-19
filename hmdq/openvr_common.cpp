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

#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include "openvr_common.h"

namespace openvr {

//  globals
//------------------------------------------------------------------------------
//  Eye nomenclature
const heyes_t EYES = {{vr::Eye_Left, LEYE}, {vr::Eye_Right, REYE}};

//  generic functions
//------------------------------------------------------------------------------
//  Return the version of the OpenVR API used in the build.
std::tuple<uint32_t, uint32_t, uint32_t> get_sdk_ver()
{
    return {vr::k_nSteamVRVersionMajor, vr::k_nSteamVRVersionMinor,
            vr::k_nSteamVRVersionBuild};
}

//  Resolve property tag enum from the type name.
vr::PropertyTypeTag_t get_ptag_from_ptype(const std::string& ptype)
{
    if (ptype == "Float") {
        return vr::k_unFloatPropertyTag;
    }
    else if (ptype == "Int32") {
        return vr::k_unInt32PropertyTag;
    }
    else if (ptype == "Uint64") {
        return vr::k_unUint64PropertyTag;
    }
    else if (ptype == "Bool") {
        return vr::k_unBoolPropertyTag;
    }
    else if (ptype == "String") {
        return vr::k_unStringPropertyTag;
    }
    else if (ptype == "Matrix34") {
        return vr::k_unHmdMatrix34PropertyTag;
    }
    else if (ptype == "Matrix44") {
        return vr::k_unHmdMatrix44PropertyTag;
    }
    else if (ptype == "Vector2") {
        return vr::k_unHmdVector2PropertyTag;
    }
    else if (ptype == "Vector3") {
        return vr::k_unHmdVector3PropertyTag;
    }
    else if (ptype == "Vector4") {
        return vr::k_unHmdVector4PropertyTag;
    }
    else if (ptype == "Quad") {
        return vr::k_unHmdQuadPropertyTag;
    }
    else {
        return vr::k_unInvalidPropertyTag;
    }
}

//  Return {<str:base_name>, <str:type_name>, <enum:type>, <bool:array>}
std::tuple<std::string, std::string, vr::PropertyTypeTag_t, bool>
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
    const auto typ_name = pname.substr(rpos1 + 1);
    return {basename, typ_name, get_ptag_from_ptype(ptype), is_array};
}

//  Parse OpenVR JSON API definition, where jd = json.load("openvr_api.json")
json parse_json_oapi(const json& jd)
{
    json tdprops;
    json tdcls;
    for (const auto& e : jd["enums"]) {
        if (e["enumname"].get<std::string>() == "vr::ETrackedDeviceProperty") {
            for (const auto& v : e["values"]) {
                const auto name = v["name"].get<std::string>();
                // val type is actually vr::ETrackedDeviceProperty
                const auto val = std::stoi(v["value"].get<std::string>());
                const auto cat = static_cast<int>(val) / 1000;
                tdprops[std::to_string(cat)][std::to_string(val)] = name;
                tdprops["name2id"][name] = val;
            }
        }
        else if (e["enumname"].get<std::string>() == "vr::ETrackedDeviceClass") {
            for (const auto& v : e["values"]) {
                auto name = v["name"].get<std::string>();
                // val type is actually vr::ETrackedDeviceClass
                const auto val = std::stoi(v["value"].get<std::string>());
                const auto fs = name.find('_');
                if (fs != std::string::npos) {
                    name = name.substr(fs + 1, std::string::npos);
                }
                tdcls[std::to_string(val)] = name;
            }
        }
    }
    return json({{"classes", tdcls}, {"props", tdprops}});
}

} // namespace openvr
