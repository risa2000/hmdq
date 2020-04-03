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

#include <OVR_CAPI.h>

#include "jkeys.h"
#include "oculus_config.h"
#include "oculus_props.h"

namespace oculus {

//  OculusVR defaults
//------------------------------------------------------------------------------
static constexpr auto INIT_FLAGS = ovrInit_Invisible;

// clang-format off
static const json VERB_PROPS = {
    {Prop::HmdType_Uint32, 0},
    {Prop::ProductName_String, 0},
    {Prop::Manufacturer_String, 0},
    {Prop::VendorId_Uint16, 2},
    {Prop::ProductId_Uint16, 2},
    {Prop::SerialNumber_String, 0},
    {Prop::FirmwareMajor_Uint16, 2},
    {Prop::FirmwareMinor_Uint16, 2},
    {Prop::AvailableHmdCaps_Uint32, 2},
    {Prop::DefaultHmdCaps_Uint32, 2},
    {Prop::AvailableTrackingCaps_Uint32, 2},
    {Prop::DefaultTrackingCaps_Uint32, 2},
    {Prop::DisplayRefreshRate_Float, 0},
};
// clang-format on

// clang-format off
//  currently identified properties with serial numbers
static const json ANON_PROPS = {
    Prop::SerialNumber_String,
};
// clang-format on

//  OculusVR Config class (default config)
//------------------------------------------------------------------------------
// Initialize the VR subsystem default config data
Config::Config()
{
    json cfg;
    cfg[j_init_flags] = INIT_FLAGS;
    cfg[j_verbosity][j_properties] = VERB_PROPS;
    cfg[j_anonymize][j_properties] = ANON_PROPS;
    m_pjConfig.reset(new json());
}

// Return VR subystem ID
std::string Config::get_id()
{
    return j_oculus;
}

// Return VR subystem default config
std::shared_ptr<json> Config::get_data()
{
    return m_pjConfig;
}

} // namespace oculus
