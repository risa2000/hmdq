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

#include <nlohmann/fifo_map.hpp>

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
