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

#include "fifo_map_fix.h"

//  inlined functions
//------------------------------------------------------------------------------
//  Print the property name to stdout.
inline void prop_head_out(const std::string& spid, const std::string& pname, int ind = 0,
                          int ts = 0)
{
    iprint(ind * ts, "{:4s} : {:s} = ", spid, pname);
}

//  functions (miscellanous)
//------------------------------------------------------------------------------
//  Print miscellanous info.
void print_misc(const json& jd, const char* prog_name, const char* prog_desc, int verb,
                int ind, int ts);

//  functions (devices and properties)
//------------------------------------------------------------------------------
//  Print enumerated devices.
void print_devs(const json& api, const json& devs, int ind, int ts);

//  Print (non-error) value of an Array type property.
void print_array_type(const std::string& pname, const json& pval, int ind, int ts);

//  Print device properties.
void print_dev_props(const json& api, const json& dprops, int verb, int ind, int ts);

//  Print all properties for all devices.
void print_all_props(const json& api, const json& props, int verb, int ind, int ts);

//  functions (geometry)
//------------------------------------------------------------------------------
//  Print out the raw (tangent) LRBT values.
void print_raw_lrbt(const json& jd, int ind, int ts);

//  Print single eye FOV values in degrees.
void print_fov(const json& jd, int ind, int ts);

//  Print total stereo FOV values in degrees.
void print_fov_total(const json& jd, int ind, int ts);

//  Print view geometry (panel rotation, IPD).
void print_view_geom(const json& jd, int ind, int ts);

//  Print the hidden area mask mesh statistics.
void print_ham_mesh(const json& ham_mesh, const char* neye, int verb, int vgeom, int ind,
                    int ts);

//  Print all the info about the view geometry, calculated FOVs, hidden area mesh, etc.
void print_geometry(const json& jd, int verb, int ind, int ts);
