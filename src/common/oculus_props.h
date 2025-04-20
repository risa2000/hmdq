/******************************************************************************
 * HMDQ Tools - tools for VR headsets and other hardware introspection        *
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

constexpr const char* HmdType_Uint32 = "Prop_HmdType_Uint32";
constexpr const char* ProductName_String = "Prop_ProductName_String";
constexpr const char* Manufacturer_String = "Prop_Manufacturer_String";
constexpr const char* VendorId_Uint16 = "Prop_VendorId_Uint16";
constexpr const char* ProductId_Uint16 = "Prop_ProductId_Uint16";
constexpr const char* SerialNumber_String = "Prop_SerialNumber_String";
constexpr const char* FirmwareMajor_Uint16 = "Prop_FirmwareMajor_Uint16";
constexpr const char* FirmwareMinor_Uint16 = "Prop_FirmwareMinor_Uint16";
constexpr const char* AvailableHmdCaps_Uint32 = "Prop_AvailableHmdCaps_Uint32";
constexpr const char* DefaultHmdCaps_Uint32 = "Prop_DefaultHmdCaps_Uint32";
constexpr const char* AvailableTrackingCaps_Uint32 = "Prop_AvailableTrackingCaps_Uint32";
constexpr const char* DefaultTrackingCaps_Uint32 = "Prop_DefaultTrackingCaps_Uint32";
constexpr const char* DisplayRefreshRate_Float = "Prop_DisplayRefreshRate_Float";
constexpr const char* FrustumHFovInRadians_Float = "Prop_FrustumHFovInRadians_Float";
constexpr const char* FrustumVFovInRadians_Float = "Prop_FrustumVFovInRadians_Float";
constexpr const char* FrustumNearZInMeters_Float = "Prop_FrustumNearZInMeters_Float";
constexpr const char* FrustumFarZInMeters_Float = "Prop_FrustumFarZInMeters_Float";

} // namespace Prop

} // namespace oculus
