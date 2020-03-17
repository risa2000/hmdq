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

namespace oculus {

//  property type defs
//------------------------------------------------------------------------------
namespace Prop {

constexpr const char* HmdType = "Prop_HmdType_Uint32";
constexpr const char* ProductName = "Prop_ProductName_String";
constexpr const char* Manufacturer = "Prop_Manufacturer_String";
constexpr const char* VendorId = "Prop_VendorId_Uint16";
constexpr const char* ProductId = "Prop_ProductId_Uint16";
constexpr const char* SerialNumber = "Prop_SerialNumber_String";
constexpr const char* FirmwareMajor = "Prop_FirmwareMajor_Uint16";
constexpr const char* FirmwareMinor = "Prop_FirmwareMinor_Uint16";
constexpr const char* AvailableHmdCaps = "Prop_AvailableHmdCaps_Uint32";
constexpr const char* DefaultHmdCaps = "Prop_DefaultHmdCaps_Uint32";
constexpr const char* AvailableTrackingCaps = "Prop_AvailableTrackingCaps_Uint32";
constexpr const char* DefaultTrackingCaps = "Prop_DefaultTrackingCaps_Uint32";
constexpr const char* DisplayRefreshRate = "Prop_DisplayRefreshRate_Float";

} // namespace Prop

} // namespace oculus
