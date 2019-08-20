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
#include <vector>

#include <fmt/format.h>

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
//  default config file name
static const char* CONF_FILE = "hmdq.conf.json";

//  config versions
//------------------------------------------------------------------------------
//  v1: Original file format defined by the tool.
//  v2: Added 'control' section for anonymizing setup.
//      Removed 'use_names' option, only "names" are suppported.
static constexpr int CFG_VERSION = 2;

//  control defaults
//------------------------------------------------------------------------------
static constexpr bool CTRL_ANONYMIZE = false;
//  currently identified properties with serial numbers
static const std::vector<vr::ETrackedDeviceProperty> PROPS_TO_HASH
    = {vr::Prop_SerialNumber_String, vr::Prop_Firmware_ProgrammingTarget_String,
       vr::Prop_ConnectedWirelessDongle_String,
       vr::Prop_AllWirelessDongleDescriptions_String};

//  verbosity defaults
//------------------------------------------------------------------------------
static constexpr int VERB_SIL = -1;
static constexpr int VERB_DEF = 0;
static constexpr int VERB_GEOM = 1;
static constexpr int VERB_MAX = 3;
static constexpr int VERB_ERR = 4;

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

//  local functions
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
    fmt::print("Writing config: \"{:s}\"\n", cfile.u8string());
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
static std::filesystem::path build_conf_name(const std::string& argv0)
{
    auto relative = std::filesystem::relative(
        std::filesystem::absolute(std::filesystem::u8path(argv0)));
    relative.replace_extension(".conf.json");
    return relative;
}

//  exported functions
//------------------------------------------------------------------------------
//  Initialize config options either from the file or from the defaults.
bool init_config(const std::string& argv0)
{
    auto cfile = build_conf_name(argv0);
    g_cfg = load_config(cfile);
    if (g_cfg.empty()) {
        g_cfg = build_config(cfile);
    }
    else {
        const auto cfg_ver = g_cfg["meta"]["cfg_ver"].get<int>();
        if (cfg_ver != CFG_VERSION) {
            fmt::print(stderr,
                       "The existing configuration file (\"{:s}\") has a different "
                       "version ({:d})\n"
                       "than what the tool supports ({:d}).\n",
                       cfile.u8string(), cfg_ver, CFG_VERSION);
            fmt::print(stderr,
                       "Please rename the old one, let the new one generate, and then "
                       "merge the changes.\n");
            return false;
        }
    }
    return true;
}
