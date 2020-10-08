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

#include "except.h"
#include "jkeys.h"
#include "json_proxy.h"

#include <filesystem>
#include <string>
#include <vector>

//  globals
//------------------------------------------------------------------------------
//  Checksum pre-defs
constexpr int CHKSUM_BITSIZE = 128;

//  Anonymizing pre-defs
constexpr auto ANON_BITSIZE = 96;

//  JSON file I/O
//------------------------------------------------------------------------------
//  Extract relevant data from OpenVR API
json read_json(const std::filesystem::path& inpath);

// Save JSON data into file with indentation.
void write_json(const std::filesystem::path& outpath, const json& jdata, int indent);

//  JSON data manipulation
//------------------------------------------------------------------------------
//  Remove all properties with errors reported from the dict.
void purge_jdprops_errors(json& jd);

//  Add an error message to JSON item
void add_error(json& jd, const char* msg);

//  Add an error message to JSON item
void add_error(json& jd, const std::string& msg);

//  Add an error message to JSON item in an array container
void add_error_array(json& jd, const char* msg);

//  Check for the error in JSON item
inline bool has_error(const json& jd)
{
    return jd.contains(ERROR_PREFIX);
}

//  Return the error
inline json get_error(const json& jd)
{
    return jd.at(ERROR_PREFIX);
}

//  Return the error message
std::string get_error_msg(const json& jd);

//  Make error object
inline json make_error_obj(const std::string& msg)
{
    return json::object({{ERROR_PREFIX, msg}});
}

//  Anonymize functions
//------------------------------------------------------------------------------
//  Anonymize the message in `in` to `out`
void anonymize(std::vector<char>& out, const std::vector<char>& in);

//  Anonymize properties in JSON list
void anonymize_jdprops(json& jdprops, const std::vector<std::string>& anon_prop_names,
                       const std::vector<std::string>& seed_prop_names);

//  Checksum functions
//------------------------------------------------------------------------------
//  Calculate the hash over the JSON string dump using Blake2b(CHKSUM_BITSIZE).
//  The returned value is an upper case string of a binhex encoded hash.
std::string calculate_checksum(const json& jd);

//  Verify the checksum in the JSON dics (if there is one)
bool verify_checksum(const json& jd);

//  Add secure checksum to the JSON dict.
inline void add_checksum(json& jd)
{
    // just do it
    jd[j_checksum] = calculate_checksum(jd);
}

//  Verify if the file has the checksum written in it
inline bool has_checksum(const json& jd)
{
    return jd.count(j_checksum);
}
