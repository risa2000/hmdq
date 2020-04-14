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

#include <openvr/openvr.h>

#include "jkeys.h"
#include "openvr_common.h"

namespace openvr {

//  globals
//------------------------------------------------------------------------------
//  Eye nomenclature
const heyes_t EYES = {{vr::Eye_Left, j_leye}, {vr::Eye_Right, j_reye}};

//  generic functions
//------------------------------------------------------------------------------
//  Return the version of the OpenVR API used in the build.
std::tuple<uint32_t, uint32_t, uint32_t> get_sdk_ver()
{
    return {vr::k_nSteamVRVersionMajor, vr::k_nSteamVRVersionMinor,
            vr::k_nSteamVRVersionBuild};
}

//  Parse OpenVR JSON API definition, where jd = json.load("openvr_api.json")
json parse_json_oapi(const json& jd)
{
    json tdprops;
    json tdcls;
    for (const auto& e : jd[j_enums]) {
        if (e[j_enumname].get<std::string>() == "vr::ETrackedDeviceProperty") {
            for (const auto& v : e[j_values]) {
                const auto name = v[j_name].get<std::string>();
                // val type is actually vr::ETrackedDeviceProperty
                const auto val = std::stoi(v[j_value].get<std::string>());
                const auto cat = static_cast<int>(val) / 1000;
                tdprops[std::to_string(cat)][std::to_string(val)] = name;
                tdprops[j_name2id][name] = val;
            }
        }
        else if (e[j_enumname].get<std::string>() == "vr::ETrackedDeviceClass") {
            for (const auto& v : e[j_values]) {
                auto name = v[j_name].get<std::string>();
                // val type is actually vr::ETrackedDeviceClass
                const auto val = std::stoi(v[j_value].get<std::string>());
                const auto fs = name.find('_');
                if (fs != std::string::npos) {
                    name = name.substr(fs + 1, std::string::npos);
                }
                tdcls[std::to_string(val)] = name;
            }
        }
    }
    return json({{j_classes, tdcls}, {j_properties, tdprops}});
}

//  Convert common property types to OpenVR property types
vr::PropertyTypeTag_t ptype_to_ptag(basevr::PropType ptype)
{
    switch (ptype) {
        case basevr::PropType::Float:
            return vr::k_unFloatPropertyTag;
        case basevr::PropType::Double:
            return vr::k_unDoublePropertyTag;
        case basevr::PropType::Int32:
            return vr::k_unInt32PropertyTag;
        case basevr::PropType::Uint64:
            return vr::k_unUint64PropertyTag;
        case basevr::PropType::Bool:
            return vr::k_unBoolPropertyTag;
        case basevr::PropType::String:
            return vr::k_unStringPropertyTag;
        case basevr::PropType::Vector2:
            return vr::k_unHmdVector2PropertyTag;
        case basevr::PropType::Vector3:
            return vr::k_unHmdVector3PropertyTag;
        case basevr::PropType::Vector4:
            return vr::k_unHmdVector4PropertyTag;
        case basevr::PropType::Matrix34:
            return vr::k_unHmdMatrix34PropertyTag;
        case basevr::PropType::Matrix44:
            return vr::k_unHmdMatrix44PropertyTag;
        case basevr::PropType::Quad:
            return vr::k_unHmdQuadPropertyTag;
        default:
            return vr::k_unInvalidPropertyTag;
    }
}

} // namespace openvr
