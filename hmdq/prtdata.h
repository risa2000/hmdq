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

#include "basecomp.h"
#include "prtdef.h"

#include "fifo_map_fix.h"

//  functions (miscellanous)
//------------------------------------------------------------------------------
//  Print header (displayed when the execution starts) needs verbosity=silent
void print_header(const char* prog_name, const char* prog_ver, const char* prog_desc,
                  int verb, int ind, int ts);

//  Print miscellanous info.
void print_misc(const json& jd, const char* prog_name, int verb, int ind, int ts);

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

//  functions (all print)
//------------------------------------------------------------------------------
//  Print the complete data file.
void print_all(const pmode selected, const json& out, const procbuff_t& processors,
               int verb, int ind, int ts);
