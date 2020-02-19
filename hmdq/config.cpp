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

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <filesystem>
#include <fstream>
#include <vector>

#include <fmt/format.h>

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
//  v3: Changed `hmdq_ver` key to `prog_ver` key.
//  v4: Added Prop_RegisteredDeviceType_String to anonymized props.
//  v5: Moved OpenVR settings into "openvr" section.
static constexpr int CFG_VERSION = 5;

//  control defaults
//------------------------------------------------------------------------------
//  anonymize sensitive data by default
static constexpr bool CTRL_ANONYMIZE = false;

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

//  Build config meta data
static json build_meta()
{
    json res;
    res["cfg_ver"] = CFG_VERSION;
    res["prog_ver"] = PROG_VERSION;
    return res;
}

//  Build (default) config and write it into JSON file.
static json build_config(const std::filesystem::path& cfile, const cfgbuff_t& cfgs)
{
    json jd;
    jd["meta"] = build_meta();
    jd["control"] = build_control();
    jd["format"] = build_format();
    jd["verbosity"] = build_verbosity();
    for (auto& cfg : cfgs) {
        jd[cfg->get_id()] = cfg->get_data();
    }
    write_config(cfile, jd);
    return jd;
}

//  Build config file name (or use the default one)
static std::filesystem::path build_conf_name(const std::filesystem::path& argv0)
{
    auto conf_name = argv0;
    return conf_name.replace_extension(".conf.json");
}

//  exported functions
//------------------------------------------------------------------------------
//  Initialize config options either from the file or from the defaults.
bool init_config(const std::filesystem::path& argv0, const cfgbuff_t& cfgs)
{
    auto cfile = build_conf_name(argv0);
    g_cfg = load_config(cfile);
    if (g_cfg.empty()) {
        g_cfg = build_config(cfile, cfgs);
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
