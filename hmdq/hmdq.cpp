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

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <ctime>
#include <filesystem>
#include <fstream>

#include <clipp.h>

#include <botan/version.h>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include "config.h"
#include "except.h"
#include "fmthlp.h"
#include "gitversion.h"
#include "jtools.h"
#include "misc.h"
#include "osver.h"
#include "ovrhlp.h"
#include "prtdata.h"

#include "fifo_map_fix.h"

//  typedefs
//------------------------------------------------------------------------------
//  mode of operation
enum class mode { geom, props, all, info, help };

//  locals
//------------------------------------------------------------------------------
static constexpr int IND = 0;

//  log versions
//------------------------------------------------------------------------------
//  v1: Original file format defined by the tool.
//  v2: Added secure checksum to the end of the file.
//  v3: Added new section 'openvr'.
static constexpr int LOG_VERSION = 3;

//  functions
//------------------------------------------------------------------------------
//  Print version and other usefull info.
void print_info(int ind = 0, int ts = 0)
{
    const auto sf = ind * ts;
    const auto sf1 = (ind + 1) * ts;
    constexpr int tf1 = 8;
    iprint(sf, "{:s} version {:s} - {:s}\n", HMDQ_NAME, HMDQ_VERSION, HMDQ_DESCRIPTION);
    fmt::print("\n");
    iprint(sf, "build info:\n");
    iprint(sf1, "{:>{}s}: {:s}\n", "git repo", tf1, HMDQ_URL);
    iprint(sf1, "{:>{}s}: {:s}\n", "git ver.", tf1, GIT_REPO_VERSION);
    iprint(sf1, "{:>{}s}: {:s} version {:s} ({:s})\n", "compiler", tf1, CXX_COMPILER_ID,
           CXX_COMPILER_VERSION, CXX_COMPILER_ARCHITECTURE_ID);
    iprint(sf1, "{:>{}s}: {:s} ({:s})\n", "host", tf1, HOST_SYSTEM,
           HOST_SYSTEM_PROCESSOR);
    iprint(sf1, "{:>{}s}: {:s}\n", "date", tf1, TIMESTAMP);
    fmt::print("\n");
    iprint(sf, "using libraries:\n");
    iprint(sf1, "{:s} ({:s})\n", "muellan/clip", "https://github.com/muellan/clipp");
    iprint(sf1, "{:s} {:s} ({:s})\n", "nlohmann/json", NLOHMANN_JSON_VERSION,
           "https://github.com/nlohmann/json");
    iprint(sf1, "{:s} {:s} ({:s})\n", "QuantStack/xtl", XTL_VERSION,
           "https://github.com/QuantStack/xtl");
    iprint(sf1, "{:s} {:s} ({:s})\n", "QuantStack/xtensor", XTENSOR_VERSION,
           "https://github.com/QuantStack/xtensor");
    iprint(sf1, "{:s} {:s} ({:s})\n", "randombit/botan", Botan::short_version_string(),
           "https://github.com/randombit/botan");
    iprint(sf1, "{:s} {:d}.{:d}.{:d} ({:s})\n", "fmtlib/fmt", FMT_VERSION / 10000,
           (FMT_VERSION % 10000) / 100, FMT_VERSION % 100,
           "https://github.com/fmtlib/fmt");
}

//  Return some miscellanous info about the app and the OS.
json get_misc()
{
    std::tm tm;

    const std::time_t t = std::time(nullptr);
    localtime_s(&tm, &t);

    json res;
    res["time"] = fmt::format("{:%F %T}", tm);
    res["hmdq_ver"] = HMDQ_VERSION;
    res["log_ver"] = LOG_VERSION;
    res["os_ver"] = get_os_ver();
    return res;
}

//  Return some info about OpenVR.
json get_openvr(vr::IVRSystem* vrsys)
{
    json res;
    res["rt_path"] = get_vr_runtime_path();
    res["rt_ver"] = get_openvr_ver(vrsys);
    return res;
}

//  main runner
int run(mode selected, const std::string& api_json, const std::string& out_json,
        bool anon, int verb, int ind, int ts)
{
    // initialize config values
    const auto json_indent = g_cfg["format"]["json_indent"].get<int>();
    const auto app_type = g_cfg["openvr"]["app_type"].get<vr::EVRApplicationType>();
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    const auto vsil = g_cfg["verbosity"]["silent"].get<int>();
    const auto verr = g_cfg["verbosity"]["error"].get<int>();

    // print the execution header
    print_header(HMDQ_NAME, HMDQ_VERSION, HMDQ_DESCRIPTION, verb, ind, ts);

    // make sure that OpenVR API file (default, or specified) exists
    std::filesystem::path apath(api_json);
    if (!std::filesystem::exists(apath)) {
        auto msg
            = fmt::format("OpenVR API JSON file not found: \"{:s}\"", apath.string());
        throw hmdq_error(msg);
    }

    // read JSON API def
    std::ifstream jfa(apath);
    json oapi;
    jfa >> oapi;

    // parse the API file to hmdq used json (dict)
    const json api = parse_json_oapi(oapi);

    // output
    json out;

    // get the miscellanous (system and app) data
    out["misc"] = get_misc();
    print_misc(out["misc"], HMDQ_NAME, verb, ind, ts);

    // get OpenVR runtime path (sanity check)
    const auto vr_rt_path = get_vr_runtime_path();
    // check if HMD is present (sanity check)
    is_hmd_present();
    // initialize OpenVR runtime (IVRSystem)
    auto vrsys = init_vrsys(app_type);

    // get some data about the OpenVR system
    out["openvr"] = get_openvr(vrsys);
    print_openvr(out["openvr"], verb, ind, ts);
    if (verb >= vdef)
        fmt::print("\n");

    // get all the properties
    auto tverb = (selected == mode::props || selected == mode::all) ? verb : vsil;
    const hdevlist_t devs = enum_devs(vrsys);
    out["devices"] = devs;
    if (tverb >= vdef) {
        print_devs(api, out["devices"], ind, ts);
        fmt::print("\n");
    }

    out["properties"] = get_all_props(vrsys, devs, api, anon);
    print_all_props(api, out["properties"], tverb, ind, ts);
    if (tverb >= vdef)
        fmt::print("\n");

    // get all the geometry
    tverb = (selected == mode::geom || selected == mode::all) ? verb : vsil;
    out["geometry"] = get_geometry(vrsys);
    print_geometry(out["geometry"], tverb, ind, ts);
    if (tverb >= vdef)
        fmt::print("\n");

    // dump the data into the optional JSON file
    if (out_json.size()) {
        // if verbosity is not high enough (verr + 1) purge errors
        if (verb <= verr) {
            purge_errors(out["properties"]);
        }
        // add the checksum
        add_checksum(out);
        // save the JSON with indentation
        std::filesystem::path opath(out_json);
        std::ofstream jfo(opath);
        jfo << out.dump(json_indent);
    }

    vr::VR_Shutdown();
    return 0;
}

//  wrapper for main runner to deal with domestic exceptions
int run_wrapper(mode selected, const std::string& api_json, const std::string& out_json,
                bool anon, int verb, int ind, int ts)
{
    int res = 0;
    try {
        res = run(selected, api_json, out_json, anon, verb, ind, ts);
    }
    catch (hmdq_error e) {
        fmt::print(stderr, ERR_MSG_FMT_OUT, e.what());
        res = 1;
    }
    return res;
}

int main(int argc, char* argv[])
{
    using namespace clipp;
    const auto OPENVR_API_JSON = "openvr_api.json";

    // init global config before anything else
    const auto cfg_ok = init_config(argc, argv);
    if (!cfg_ok)
        return 1;

    const auto ts = g_cfg["format"]["cli_indent"].get<int>();
    const auto ind = IND;

    // defaults for the arguments
    auto verb = g_cfg["verbosity"]["default"].get<int>();
    auto anon = g_cfg["control"]["anonymize"].get<bool>();

    mode selected = mode::all;
    std::string api_json = OPENVR_API_JSON;
    std::string out_json;
    // custom help texts
    const auto verb_help = fmt::format("verbosity level [{:d}]", verb);
    const auto api_json_help
        = fmt::format("OpenVR API JSON definition file [\"{:s}\"]", api_json);
    const auto anon_help
        = fmt::format("anonymize serial numbers in the output [{}]", anon);

    // use this as a workaround to accept "empty" command, first parse
    // all together (cli_cmds, cli_opts) then only cli_opts, to accepts
    // only the options without the command.
    clipp::group cli_opts
        = ((option("-a", "--api_json") & value("name", api_json)) % api_json_help,
           (option("-o", "--out_json") & value("name", out_json)) % "JSON output file",
           (option("-v", "--verb").set(verb, 1) & opt_value("level", verb)) % verb_help,
           (option("-n", "--anonymize").set(anon, !anon)) % anon_help);

    clipp::group cli_cmds
        = (command("geom").set(selected, mode::geom).doc("show only geometry data")
           | command("props")
                 .set(selected, mode::props)
                 .doc("show only device properties")
           | command("version")
                 .set(selected, mode::info)
                 .doc("show version and other info")
           | command("all").set(selected, mode::all).doc("show all data (default choice)")
           | command("help").set(selected, mode::help).doc("show this help page"));

    clipp::group cli = (cli_cmds, cli_opts);

    int res = 0;
    if (parse(argc, argv, cli)) {
        switch (selected) {
        case mode::info:
            print_info(ind, ts);
            break;
        case mode::geom:
        case mode::props:
        case mode::all:
            res = run_wrapper(selected, api_json, out_json, anon, verb, ind, ts);
            break;
        case mode::help:
            fmt::print("Usage:\n{:s}\nOptions:\n{:s}\n",
                       usage_lines(cli, HMDQ_NAME).str(), documentation(cli).str());
            break;
        }
    }
    else {
        if (parse(argc, argv, cli_opts)) {
            res = run_wrapper(mode::all, api_json, out_json, anon, verb, ind, ts);
        }
        else {
            fmt::print("Usage:\n{:s}\n", usage_lines(cli, HMDQ_NAME).str());
            res = 1;
        }
    }
    return res;
}
