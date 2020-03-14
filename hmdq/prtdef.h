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
enum pmode { geom, props, all };
