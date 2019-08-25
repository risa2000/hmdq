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
#include <string>

#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include "fifo_map_fix.h"

//  typedefs
//------------------------------------------------------------------------------
typedef std::vector<std::pair<vr::EVREye, std::string>> heyes_t;
typedef std::vector<vr::ETrackedDeviceProperty> hproplist_t;
typedef std::vector<std::pair<vr::TrackedDeviceIndex_t, vr::ETrackedDeviceClass>>
    hdevlist_t;

//  globals
//------------------------------------------------------------------------------
//  Eye nomenclature
constexpr const char* LEYE = "Left";
constexpr const char* REYE = "Right";
extern const heyes_t EYES;
//  Property IDs to seed the hash for anonymized properties.
extern const hproplist_t PROPS_TO_SEED;

//  Checksum pre-defs
constexpr const char* CHECKSUM = "checksum";
constexpr int CHKSUM_BITSIZE = 128;

//  Anonymizing pre-defs
constexpr auto ANON_BITSIZE = 96;
constexpr const char* ANON_PREFIX = "anon@";

//  Error reporting pre-defs
constexpr const char* ERROR_PREFIX = "error@";
constexpr const char* MSG_TYPE_NOT_IMPL = "{:s} type not implemented";

//  generic functions
//------------------------------------------------------------------------------
//  Return {<str:base_name>, <str:type_name>, <enum:type>, <bool:array>}
std::tuple<std::string, std::string, vr::PropertyTypeTag_t, bool>
parse_prop_name(const std::string& pname);

//  OpenVR API loader
//------------------------------------------------------------------------------
//  Parse OpenVR JSON API definition, where jd = json.load("openvr_api.json")
json parse_json_oapi(const json& jd);

//  functions
//------------------------------------------------------------------------------
//  Remove all properties with errors reported from the dict.
void purge_errors(json& jd);

//  Calculate the hash over the JSON string dump using Blake2b(CHKSUM_BITSIZE).
//  The returned value is an upper case string of a binhex encoded hash.
std::string calculate_checksum(const json& jd);

//  Anonymize the message in `in` to `out`
void anonymize(std::vector<char>& out, const std::vector<char>& in);

//  Anonymize the properties defined in the config file. If the values are already
//  anonymized, do nothing
void anonymize_all_props(const json& api, json& props);

//  Verify the checksum in the JSON dics (if there is one)
bool verify_checksum(const json& jd);

//  inlines
//------------------------------------------------------------------------------
//  Add secure checksum to the JSON dict.
inline void add_checksum(json& jd)
{
    // just do it
    jd[CHECKSUM] = calculate_checksum(jd);
}

//  Verify if the file has the checksum written in it
inline bool has_checksum(const json& jd)
{
    return jd.count(CHECKSUM);
}
