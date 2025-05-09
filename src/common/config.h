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

#include <common/base_classes.h>
#include <common/json_proxy.h>

#include <filesystem>
#include <vector>

//  globals
//------------------------------------------------------------------------------
extern json g_cfg;

//  functions
//------------------------------------------------------------------------------
//  Initialize config options either from the file or from the defaults.
bool init_config(const std::filesystem::path& argv0, const cfgmap_t& cfgs);
