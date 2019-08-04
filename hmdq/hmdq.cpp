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
#include <iomanip>
#include <iostream>

#include <clipp.h>

#include <botan/version.h>

#include "config.h"
#include "gitversion.h"
#include "misc.h"
#include "osver.h"
#include "ovrhlp.h"
#include "utils.h"

#include "fifo_map_fix.h"

//  typedefs
//------------------------------------------------------------------------------
//  mode of operation
enum class mode { geom, props, all, info, help };

//  locals
//------------------------------------------------------------------------------
static constexpr int IND = 0;
static constexpr int LOG_VERSION = 1;

//  functions
//------------------------------------------------------------------------------
//  Print version and other usefull info
void print_info(int ind = 0, int ts = 0)
{
    std::string sf(ts * ind, ' ');
    std::string sf1(ts * (ind + 1), ' ');
    constexpr int t1 = 10;

    std::cout << sf << HMDQ_NAME << " version " << HMDQ_VERSION << " - "
              << HMDQ_DESCRIPTION << '\n';
    std::cout << '\n';
    std::cout << sf << "build info:\n";
    std::cout << sf1 << std::setw(t1) << "git repo: " << HMDQ_URL << '\n';
    std::cout << sf1 << std::setw(t1) << "git ver.: " << GIT_REPO_VERSION << '\n';
    std::cout << sf1 << std::setw(t1) << "compiler: " << CXX_COMPILER_ID << " version "
              << CXX_COMPILER_VERSION << " (" << CXX_COMPILER_ARCHITECTURE_ID << ")\n";
    std::cout << sf1 << std::setw(t1) << "host: " << HOST_SYSTEM << " ("
              << HOST_SYSTEM_PROCESSOR << ")\n";
    std::cout << sf1 << std::setw(t1) << "date: " << TIMESTAMP << '\n';
    std::cout << '\n';
    std::cout << sf << "using libraries:\n";
    std::cout << sf1 << std::setw(t1)
              << "muellan/clip (https://github.com/muellan/clipp)\n";
    std::cout << sf1 << std::setw(t1) << "nlohmann/json " << NLOHMANN_JSON_VERSION
              << " (https://github.com/nlohmann/json)\n";
    std::cout << sf1 << std::setw(t1) << "QuantStack/xtl " << XTL_VERSION
              << " (https://github.com/QuantStack/xtl)\n";
    std::cout << sf1 << std::setw(t1) << "QuantStack/xtensor " << XTENSOR_VERSION
              << " (https://github.com/QuantStack/xtensor)\n";
    std::cout << sf1 << std::setw(t1) << "randombit/botan "
              << Botan::short_version_string()
              << " (https://github.com/randombit/botan)\n";
}

//  Return some miscellanous info about OpenVR.
json get_misc(int verb = 0, int ind = 0, int ts = 0)
{
    const std::string sf(ts * ind, ' ');
    const auto vsil = g_cfg["verbosity"]["silent"].get<int>();
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    std::stringstream stime;
    std::tm tm;

    std::time_t t = std::time(nullptr);
    localtime_s(&tm, &t);
    stime << std::put_time(&tm, "%FT%T");

    json res;
    res["time"] = stime.str();
    res["hmdq_ver"] = HMDQ_VERSION;
    res["log_ver"] = LOG_VERSION;
    res["openvr_ver"] = "n/a";
    auto os_ver = get_os_ver();
    res["os_ver"] = os_ver;
    if (verb >= vsil) {
        std::cout << sf << HMDQ_NAME << " version " << HMDQ_VERSION << " - "
                  << HMDQ_DESCRIPTION << '\n';
    }
    if (verb >= vdef) {
        std::cout << sf << "Current time: " << std::put_time(&tm, "%c") << '\n';
        std::cout << sf << "OS version: " << os_ver << '\n';
    }
    return res;
}

//  main runner
int run(mode selected, const std::string& api_json, const std::string& out_json,
        bool anon, int verb, int ind, int ts)
{
    // initialize config values
    const auto json_indent = g_cfg["format"]["json_indent"].get<int>();
    const auto app_type = g_cfg["openvr"]["app_type"].get<vr::EVRApplicationType>();
    const auto use_names = g_cfg["verbosity"]["use_names"].get<bool>();
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    const auto vsil = g_cfg["verbosity"]["silent"].get<int>();

    // make sure that OpenVR API file (default, or specified) exists
    std::filesystem::path apath(api_json);
    if (!std::filesystem::exists(apath)) {
        std::stringstream msg;
        msg << "OpenVR API JSON file not found: " << apath;
        throw hmdq_error(msg.str());
    }

    // read JSON API def
    std::ifstream jfa(apath);
    json oapi;
    jfa >> oapi;

    // parse the API file to hmdq used json (dict)
    json api = parse_json_oapi(oapi);

    // output
    json out;

    // get the miscellanous (system and app) data
    out["misc"] = get_misc(verb, ind, ts);

    // Initializing OpenVR runtime (IVRSystem)
    auto vrsys = get_vrsys(app_type, verb, ind, ts);
    if (verb >= vdef)
        std::cout << '\n';

    if (vrsys == nullptr) {
        throw hmdq_error("Cannot initialize OpenVR");
    }

    // get all the properties
    auto tverb = (selected == mode::props || selected == mode::all) ? verb : vsil;
    hdevlist_t devs = enum_devs(vrsys, api, tverb, ind, ts);
    out["devices"] = devs;
    if (tverb >= vdef)
        std::cout << '\n';

    out["properties"] = get_all_props(vrsys, devs, api, anon, use_names, tverb, ind, ts);
    if (tverb >= vdef)
        std::cout << '\n';

    // get all the geometry
    tverb = (selected == mode::geom || selected == mode::all) ? verb : vsil;
    out["geometry"] = get_eyes_geometry(vrsys, EYES, tverb, ind, ts);

    // dump the data into the optional JSON file
    if (out_json.size()) {
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
        std::cerr << "Error: " << e.what() << "\n";
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
    std::string verb_help
        = std::string("verbosity level [") + std::to_string(verb) + std::string("]");
    std::string api_json_help
        = std::string("OpenVR API JSON definition file [") + api_json + std::string("]");
    std::string anon_help = std::string("anonymize serial numbers in the output [")
        + (anon ? "true" : "false") + std::string("]");

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
            std::cout << "Usage:\n"
                      << usage_lines(cli, HMDQ_NAME) << "\nOptions:\n"
                      << documentation(cli) << '\n';
            break;
        }
    }
    else {
        if (parse(argc, argv, cli_opts)) {
            res = run_wrapper(mode::all, api_json, out_json, anon, verb, ind, ts);
        }
        else {
            std::cout << "Usage:\n" << usage_lines(cli, HMDQ_NAME) << '\n';
            res = 1;
        }
    }
    return res;
}
