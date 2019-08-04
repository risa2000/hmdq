/******************************************************************************
 * HMDQ - Query tool for an OpenVR HMD and some other hardware                *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2019, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include "config.h"
#include "misc.h"

#include "fifo_map_fix.h"

//  globals
//------------------------------------------------------------------------------
json g_cfg;

//  locals
//------------------------------------------------------------------------------
//  actual version of the config file
constexpr int CFG_VERSION = 2;
//  default config file name
static const char* CONF_FILE = "hmdq.conf.json";

//  control defaults
//------------------------------------------------------------------------------
static constexpr bool CTRL_ANONYMIZE = false;
//  currently identified properties with serial numbers
static const std::vector<vr::ETrackedDeviceProperty> PROPS_TO_HASH
    = {vr::Prop_SerialNumber_String, vr::Prop_Firmware_ProgrammingTarget_String,
       vr::Prop_ConnectedWirelessDongle_String};

//  verbosity defaults
//------------------------------------------------------------------------------
static constexpr int VERB_SIL = -1;
static constexpr int VERB_DEF = 0;
static constexpr int VERB_GEOM = 1;
static constexpr int VERB_MAX = 3;
static constexpr int VERB_ERR = 4;
static constexpr bool VERB_USE_NAMES = true;

//  formatting defaults
//------------------------------------------------------------------------------
static constexpr int JSON_INDENT = 2;
static constexpr int CLI_INDENT = 4;

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

//  functions
//------------------------------------------------------------------------------
//  Load config file, if it exists.
static json load_config(const std::filesystem::path& cfile)
{
    json jd;
    if (std::filesystem::exists(cfile)) {
        std::ifstream jfi(cfile);
        jfi >> jd;
    }
    return jd;
}

//  Write config in `jd` to `cfile`.
static void write_config(const std::filesystem::path& cfile, const json& jd)
{
    std::ofstream jf(cfile);
    std::cout << "Writing config: " << cfile << '\n';
    jf << jd.dump(JSON_INDENT);
}

//  Build default config for control settings
static json build_control()
{
    json res;
    res["anonymize"] = CTRL_ANONYMIZE;
    res["anon_props"] = PROPS_TO_HASH;
    return res;
}

//  Build default config for verbosity settings
static json build_verbosity()
{
    json res;
    res["use_names"] = VERB_USE_NAMES;
    res["silent"] = VERB_SIL;
    res["default"] = VERB_DEF;
    res["geom"] = VERB_GEOM;
    res["max"] = VERB_MAX;
    res["error"] = VERB_ERR;
    res["props"] = VERB_PROPS;
    return res;
}

//  Build default config for formatting settings
static json build_format()
{
    json res;
    res["json_indent"] = JSON_INDENT;
    res["cli_indent"] = CLI_INDENT;
    return res;
}

//  Build default config for OpenVR settings
static json build_openvr()
{
    json res;
    res["app_type"] = APP_TYPE;
    return res;
}

//  Build config meta data
static json build_meta()
{
    json res;
    res["cfg_ver"] = CFG_VERSION;
    res["hmdq_ver"] = HMDQ_VERSION;
    return res;
}

//  Build (default) config and write it into JSON file.
static json build_config(const std::filesystem::path& cfile)
{
    json jd;
    jd["meta"] = build_meta();
    jd["control"] = build_control();
    jd["format"] = build_format();
    jd["openvr"] = build_openvr();
    jd["verbosity"] = build_verbosity();
    write_config(cfile, jd);
    return jd;
}

//  Build config file name (or use the default one)
static std::filesystem::path build_conf_name(int argc, char* argv[])
{
    if (argc == 0) {
        return std::filesystem::path(CONF_FILE);
    }
    else {
        auto exepath = std::filesystem::path(argv[0]);
        return exepath.filename().replace_extension(".conf.json");
    }
}

//  Initialize config options either from the file or from the defaults.
bool init_config(int argc, char* argv[])
{
    auto cfile = build_conf_name(argc, argv);
    g_cfg = load_config(cfile);
    if (g_cfg.empty()) {
        g_cfg = build_config(cfile);
    }
    else {
        const auto cfg_ver = g_cfg["meta"]["cfg_ver"].get<int>();
        if (cfg_ver != CFG_VERSION) {
            std::cerr << "The existing configuration file (" << cfile
                      << ") has a different version (" << cfg_ver
                      << ")\nthan what the tool supports (" << CFG_VERSION << ").\n"
                      << "Please rename the old one, let the new one generate, and then "
                         "merge the changes.\n";
            return false;
        }
    }
    return true;
}
