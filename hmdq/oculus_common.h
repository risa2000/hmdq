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
#include <vector>

#include <nlohmann/fifo_map.hpp>

#include <OVR_CAPI.h>

#include "fifo_map_fix.h"

namespace oculus {

//  typedefs
//------------------------------------------------------------------------------
typedef std::vector<std::pair<ovrEyeType, std::string>> eyes_t;

//  common constants
//------------------------------------------------------------------------------
//  Controller types names
extern const nlohmann::fifo_map<int, const char*> g_bmControllerTypes;
//  HMD capabilities names
extern const nlohmann::fifo_map<int, const char*> g_bmHmdCaps;
//  Tracker capabilites names
extern const nlohmann::fifo_map<int, const char*> g_bmTrackingCaps;
//  HMD types names
extern const nlohmann::fifo_map<int, const char*> g_mHmdTypes;

//  Eye nomenclature
extern const eyes_t EYES;

} // namespace oculus

//  nlohmann/json serializers
//------------------------------------------------------------------------------
//  ovrVector2f serializers
void to_json(json& j, const ovrVector2f& v2f);
void from_json(const json& j, ovrVector2f& v2f);
//  ovrFovPort serializers
void to_json(json& j, const ovrFovPort& fovPort);
void from_json(const json& j, ovrFovPort& fovPort);
//  ovrRecti serializers
void to_json(json& j, const ovrRecti& rect);
void from_json(const json& j, ovrRecti& rect);
