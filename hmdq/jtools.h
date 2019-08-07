/******************************************************************************
 * HMDQ - Query tool for an OpenVR HMD and some other hardware                *
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

#include "fifo_map_fix.h"

//  globals
//------------------------------------------------------------------------------
//  Checksum pre-defs
constexpr const char* CHECKSUM = "checksum";
constexpr int CHKSUM_BITSIZE = 128;

//  Anonymizing pre-defs
constexpr auto ANON_BITSIZE = 96;
constexpr const char* ANON_PREFIX = "anon@";

//  Error reporting pre-defs
constexpr const char* ERROR_PREFIX = "error@";
constexpr const char* MSG_TYPE_NOT_IMPL = "{:s} type not implemented";

//  functions
//------------------------------------------------------------------------------
//  Remove all properties with errors reported from the dict.
void purge_errors(json& jd);

//  Calculate the hash over the JSON string dump using Blake2b(CHKSUM_BITSIZE).
//  The returned value is an upper case string of a binhex encoded hash.
std::string calculate_checksum(const json& jd);

//  Anonymize the message in `in` to `out`
void anonymize(std::vector<char>& out, const std::vector<char>& in);

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
