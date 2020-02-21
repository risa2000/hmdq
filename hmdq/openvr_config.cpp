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

#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include "jkeys.h"
#include "openvr_config.h"

namespace openvr {

//  OpenVR defaults
//------------------------------------------------------------------------------
static constexpr auto APP_TYPE = vr::VRApplication_Background;

// clang-format off
static const json VERB_PROPS = {
    {"Prop_TrackingSystemName_String", 0},
    {"Prop_ModelNumber_String", 0},
    {"Prop_SerialNumber_String", 0},
    {"Prop_RenderModelName_String", 0},
    {"Prop_ManufacturerName_String", 0},
    {"Prop_TrackingFirmwareVersion_String", 0},
    {"Prop_HardwareRevision_String", 0},
    {"Prop_ConnectedWirelessDongle_String", 2},
    {"Prop_DeviceIsWireless_Bool", 2},
    {"Prop_DeviceIsCharging_Bool", 2},
    {"Prop_DeviceBatteryPercentage_Float", 0},
    {"Prop_Firmware_UpdateAvailable_Bool", 2},
    {"Prop_Firmware_ManualUpdate_Bool", 2},
    {"Prop_Firmware_ManualUpdateURL_String", 2},
    {"Prop_HardwareRevision_Uint64", 2},
    {"Prop_FirmwareVersion_Uint64", 2},
    {"Prop_FPGAVersion_Uint64", 2},
    {"Prop_VRCVersion_Uint64", 2},
    {"Prop_RadioVersion_Uint64", 2},
    {"Prop_DongleVersion_Uint64", 2},
    {"Prop_DeviceProvidesBatteryStatus_Bool", 2},
    {"Prop_Firmware_ProgrammingTarget_String", 2},
    {"Prop_RegisteredDeviceType_String", 2},
    {"Prop_InputProfilePath_String", 2},
    {"Prop_SecondsFromVsyncToPhotons_Float", 2},
    {"Prop_DisplayFrequency_Float", 0},
    {"Prop_FieldOfViewLeftDegrees_Float", 2},
    {"Prop_FieldOfViewRightDegrees_Float", 2},
    {"Prop_FieldOfViewTopDegrees_Float", 2},
    {"Prop_FieldOfViewBottomDegrees_Float", 2},
    {"Prop_TrackingRangeMinimumMeters_Float", 2},
    {"Prop_TrackingRangeMaximumMeters_Float", 2},
    {"Prop_ModeLabel_String", 0}
};
// clang-format on

// clang-format off
//  currently identified properties with serial numbers
static const json ANON_PROPS = {
    "Prop_SerialNumber_String",
    "Prop_AllWirelessDongleDescriptions_String",
    "Prop_ConnectedWirelessDongle_String",
    "Prop_Firmware_ProgrammingTarget_String",
    "Prop_RegisteredDeviceType_String"
};
// clang-format on

//  OpenVR Config class (default config)
//------------------------------------------------------------------------------
// Initialize the VR subsystem default config data
Config::Config()
{
    m_jConfig[j_app_type] = APP_TYPE;
    m_jConfig[j_verbosity][j_properties] = VERB_PROPS;
    m_jConfig[j_anonymize][j_properties] = ANON_PROPS;
}

std::string Config::get_id()
{
    return j_openvr;
}

json& Config::get_data()
{
    return m_jConfig;
}

} // namespace openvr
