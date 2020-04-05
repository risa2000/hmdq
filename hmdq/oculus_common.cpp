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

#include <OVR_CAPI.h>

#include "oculus_common.h"

//  common constants
//------------------------------------------------------------------------------
//  Controller types names
// clang-format off
const nlohmann::fifo_map<int, const char*> g_bmControllerTypes = {
    {ovrControllerType_None, "None"},
    {ovrControllerType_LTouch, "LTouch"},
    {ovrControllerType_RTouch, "RTouch"},
    {ovrControllerType_Remote, "Remote"},
    {ovrControllerType_XBox, "XBox"},
    {ovrControllerType_Object0, "Object0"},
    {ovrControllerType_Object1, "Object1"},
    {ovrControllerType_Object2, "Object2"},
    {ovrControllerType_Object3, "Object3"},
};
// clang-format on

//  HMD capabilities names
const nlohmann::fifo_map<int, const char*> g_bmHmdCaps = {
    /// <B>(read only)</B> Specifies that the HMD is a virtual debug device.
    {ovrHmdCap_DebugDevice, "DebugDevice"},
};

//  Tracker capabilites names
const nlohmann::fifo_map<int, const char*> g_bmTrackingCaps = {
    {ovrTrackingCap_Orientation, "Orientation"},
    {ovrTrackingCap_MagYawCorrection, "MagYawCorrection"},
    {ovrTrackingCap_Position, "Position"},
};

//  HMD types names
const nlohmann::fifo_map<int, const char*> g_mHmdTypes = {
    {ovrHmd_None, "None"},       {ovrHmd_DK1, "DK1"},   {ovrHmd_DKHD, "DKHD"},
    {ovrHmd_DK2, "DK2"},         {ovrHmd_CB, "CB"},     {ovrHmd_Other, "Other"},
    {ovrHmd_E3_2015, "E3_2015"}, {ovrHmd_ES06, "ES06"}, {ovrHmd_ES09, "ES09"},
    {ovrHmd_ES11, "ES11"},       {ovrHmd_CV1, "CV1"},   {ovrHmd_RiftS, "RiftS"},
};
