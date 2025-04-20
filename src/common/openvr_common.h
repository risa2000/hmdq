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

#include <string>
#include <tuple>
#include <vector>

#include <openvr/openvr.h>

#include "base_common.h"

#include "json_proxy.h"

namespace openvr {

//  typedefs
//------------------------------------------------------------------------------
typedef std::vector<std::pair<vr::EVREye, std::string>> heyes_t;
typedef std::vector<vr::ETrackedDeviceProperty> hproplist_t;
typedef std::pair<vr::TrackedDeviceIndex_t, vr::ETrackedDeviceClass> hdevpair_t;
typedef std::vector<hdevpair_t> hdevlist_t;

//  globals
//------------------------------------------------------------------------------
//  Eye nomenclature
extern const heyes_t EYES;

//  generic functions
//------------------------------------------------------------------------------
//  Return the version of the OpenVR API used in the build.
std::tuple<uint32_t, uint32_t, uint32_t> get_sdk_ver();

//  Parse OpenVR JSON API definition, where jd = json.load("openvr_api.json")
json parse_json_oapi(const json& jd);

//  Convert common property types to OpenVR property types
vr::PropertyTypeTag_t ptype_to_ptag(basevr::PropType ptype);

} // namespace openvr
