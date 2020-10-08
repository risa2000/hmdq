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

//  globals
//------------------------------------------------------------------------------
//  Error reporting pre-defs
constexpr const char* MSG_TYPE_NOT_IMPL = "{:s} type not implemented";

//  Error message format
constexpr const char* ERR_MSG_FMT_JSON = "[error: {:s}]\n";
constexpr const char* ERR_MSG_FMT_OUT = "Error: {:s}\n";

//  typedefs
//------------------------------------------------------------------------------
//  print mode
enum class pmode { geom, props, all };

//  Command line options passed to the program
struct print_options {
    print_options()
        : anonymize(false), oculus(true), openvr(true), ovr_def_fov(true),
          ovr_max_fov(false), dbg_raw_in(false), dbg_raw_out(false), verbosity(0),
          mode(pmode::all)
    {}
    bool anonymize; // anonymize the sensitive data
    bool oculus; // show Oculus data
    bool openvr; // show OpenVR data
    bool ovr_def_fov; // show Oculus default FOV
    bool ovr_max_fov; // show Oculus max FOV
    bool dbg_raw_in; // read collected data from JSON file into the processor
    bool dbg_raw_out; // write collected data into JSON file without any processing
    int verbosity; // output verbosity
    pmode mode; // print mode
};
