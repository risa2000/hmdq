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

#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>

#include <clipp.h>

#include <botan/version.h>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include "compat.h"
#include "config.h"
#include "except.h"
#include "fmthlp.h"
#include "gitversion.h"
#include "jkeys.h"
#include "jtools.h"
#include "misc.h"
#include "oculus_collector.h"
#include "oculus_config.h"
#include "oculus_processor.h"
#include "openvr_collector.h"
#include "openvr_common.h"
#include "openvr_config.h"
#include "openvr_processor.h"
#include "prtdata.h"
#include "wintools.h"

#include "fifo_map_fix.h"

//  typedefs
//------------------------------------------------------------------------------
//  mode of operation
enum class mode { geom, props, all, info, help };

//  locals
//------------------------------------------------------------------------------
static constexpr int IND = 0;
static constexpr unsigned int CP_UTF8 = 65001;
const auto OPENVR_API_JSON = "openvr_api.json";

//  log versions
//------------------------------------------------------------------------------
//  v1: Original file format defined by the tool.
//  v2: Added secure checksum to the end of the file.
//  v3: Added new section 'openvr'.
//  v4: IPD now reported in meters (was mm).
//  v5: OpenVR related data moved into 'openvr' section.
static constexpr int LOG_VERSION = 5;

//  functions
//------------------------------------------------------------------------------
//  Print version and other usefull info.
void print_info(int ind = 0, int ts = 0)
{
    const auto sf = ind * ts;
    const auto sf1 = (ind + 1) * ts;
    constexpr int tf1 = 8;
    iprint(sf, "{:s} version {:s} - {:s}\n", PROG_NAME, PROG_VERSION, PROG_DESCRIPTION);
    fmt::print("\n");
    iprint(sf, "build info:\n");
    iprint(sf1, "{:>{}s}: {:s}\n", "git repo", tf1, PROG_URL);
    iprint(sf1, "{:>{}s}: {:s}\n", "git ver.", tf1, GIT_REPO_VERSION);
    iprint(sf1, "{:>{}s}: {:s} version {:s} ({:s})\n", "compiler", tf1, CXX_COMPILER_ID,
           CXX_COMPILER_VERSION, CXX_COMPILER_ARCHITECTURE_ID);
    iprint(sf1, "{:>{}s}: {:s} ({:s})\n", "host", tf1, HOST_SYSTEM,
           HOST_SYSTEM_PROCESSOR);
    iprint(sf1, "{:>{}s}: {:s}\n", "date", tf1, TIMESTAMP);
    fmt::print("\n");
    iprint(sf, "using libraries:\n");
    constexpr const char* libver_nover_fmt = "{0:s} (https://github.com/{0:s})\n";
    constexpr const char* libver_str_fmt = "{0:s} {1:s} (https://github.com/{0:s})\n";
    constexpr const char* libver_num_fmt
        = "{0:s} {1:d}.{2:d}.{3:d} (https://github.com/{0:s})\n";
    iprint(sf1, libver_nover_fmt, "muellan/clip");
    iprint(sf1, libver_str_fmt, "nlohmann/json", NLOHMANN_JSON_VERSION);
    iprint(sf1, libver_str_fmt, "QuantStack/xtl", XTL_VERSION);
    iprint(sf1, libver_str_fmt, "QuantStack/xtensor", XTENSOR_VERSION);
    iprint(sf1, libver_str_fmt, "randombit/botan", Botan::short_version_string());
    iprint(sf1, libver_num_fmt, "fmtlib/fmt", FMT_VERSION / 10000,
           (FMT_VERSION % 10000) / 100, FMT_VERSION % 100);
    const auto [vmaj, vmin, vbuild] = openvr::get_sdk_ver();
    iprint(sf1, libver_num_fmt, "ValveSoftware/openvr", vmaj, vmin, vbuild);
    iprint(sf1, "{0:s} {1:s}\n", "Oculus/LibOVR", OVR_VERSION_STRING);
}

//  Return some miscellanous info about the app and the OS.
json get_misc()
{
    std::tm tm;

    const std::time_t t = std::time(nullptr);
    localtime_s(&tm, &t);

    json res;
    res[j_time] = fmt::format("{:%F %T}", tm);
    res[j_hmdq_ver] = PROG_VERSION;
    res[j_log_ver] = LOG_VERSION;
    res[j_os_ver] = get_os_ver();
    return res;
}

//  Translate the tool selected mode into print mode.
static pmode mode2pmode(const mode selected)
{
    switch (selected) {
        case mode::props:
            return pmode::props;
        case mode::geom:
            return pmode::geom;
        case mode::all:
            return pmode::all;
        default:
            HMDQ_EXCEPTION(fmt::format("mode2pmode({}) is undefined", selected));
    }
}

//  main runner
int run(mode selected, const std::filesystem::path& api_json,
        const std::filesystem::path& out_json, bool anon, int verb, int ind, int ts)
{
    // initialize config values
    const auto json_indent = g_cfg[j_format][j_json_indent].get<int>();
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();
    const auto verr = g_cfg[j_verbosity][j_error].get<int>();

    // print the execution header
    print_header(PROG_NAME, PROG_VERSION, PROG_DESCRIPTION, verb, ind, ts);
    if (verb >= vdef)
        fmt::print("\n");

    // output
    json out;

    // get the miscellanous (system and app) data
    out[j_misc] = get_misc();

    // collector buffer
    colmap_t collectors;
    // processor buffer
    procmap_t processors;

    // create all VR subsystem interfaces
    // OpenVR collector
    const auto openvr_app_type
        = g_cfg[j_openvr][j_app_type].get<vr::EVRApplicationType>();
    auto openvr_collector = new openvr::Collector(api_json, openvr_app_type);
    auto openvr_processor = new openvr::Processor(openvr_collector->get_xapi(),
                                                  openvr_collector->get_data());
    collectors.emplace(openvr_collector->get_id(), openvr_collector);
    processors.emplace(openvr_processor->get_id(), openvr_processor);

    // Oculus VR collector
    const auto init_flags = g_cfg[j_oculus][j_init_flags].get<ovrInitFlags>();
    auto oculus_collector = new oculus::Collector(init_flags);
    auto oculus_processor = new oculus::Processor(oculus_collector->get_data());
    collectors.emplace(oculus_collector->get_id(), oculus_collector);
    processors.emplace(oculus_processor->get_id(), oculus_processor);

    for (auto& [col_id, col] : collectors) {
        if (col->try_init()) {
            col->collect();
            // if there is a processor registered run it now
            if (processors.find(col_id) != processors.end()) {
                auto proc = processors[col_id].get();
                proc->init();
                proc->calculate();
                if (anon) {
                    proc->anonymize();
                }
            }
        }
    }

    print_all(mode2pmode(selected), out, processors, verb, ind, ts);

    // if verbosity is not high enough (verr + 1) purge the temporary data
    for (auto& [proc_id, proc] : processors) {
        if (verb <= verr) {
            proc->purge();
        }
    }

    // put the collected data into the output JSON
    for (auto& [col_id, col] : collectors) {
        if (!col->get_data()->is_null())
            out[col_id] = *col->get_data();
    }

    // dump the data into the optional JSON file
    if (!out_json.empty()) {
        // add the checksum
        add_checksum(out);
        // save the JSON file with indentation
        write_json(out_json, out, json_indent);
    }

    return 0;
}

//  wrapper for main runner to deal with domestic exceptions
int run_wrapper(mode selected, const std::filesystem::path& api_json,
                const std::filesystem::path& out_json, bool anon, int verb, int ind,
                int ts)
{
    int res = 0;
    try {
        res = run(selected, api_json, out_json, anon, verb, ind, ts);
    }
    catch (hmdq_error e) {
        fmt::print(stderr, ERR_MSG_FMT_OUT, e.what());
        res = 1;
    }
    catch (std::runtime_error e) {
        fmt::print(stderr, "{}\n", e.what());
    }
    return res;
}

int main(int argc, char* argv[])
{
    using namespace clipp;

    // Set UTF-8 code page for the console if available
    // if not, print UTF-8 strings to the console anyway.
    init_console_cp();
    set_console_cp(CP_UTF8);

    // get command line args in Windows Unicode and in UTF-8
    const auto u8args = get_u8args();
    if (u8args.size() == 0) {
        throw hmdq_error("Cannot get the command line arguments");
    }
    // print_u8args(u8args);

    // init global config before anything else
    cfgmap_t cfgs;
    auto openvr_config = new openvr::Config();
    cfgs.emplace(openvr_config->get_id(), openvr_config);
    auto oculus_config = new oculus::Config();
    cfgs.emplace(oculus_config->get_id(), oculus_config);

    const auto cfg_ok = init_config(get_full_prog_path(), cfgs);
    if (!cfg_ok)
        return 1;

    const auto ts = g_cfg[j_format][j_cli_indent].get<int>();
    const auto ind = IND;

    // defaults for the arguments
    auto verb = g_cfg[j_verbosity][j_default].get<int>();
    auto anon = g_cfg[j_control][j_anonymize].get<bool>();

    // default command is 'all'
    mode selected = mode::all;

    // build relative path to OPENVR_API_JSON file
    std::filesystem::path api_json_path = get_full_prog_path();
    api_json_path.replace_filename(OPENVR_API_JSON);
    auto api_json = u8str2str(api_json_path.u8string());

    std::string out_json;
    // custom help texts
    const auto verb_help = fmt::format("verbosity level [{:d}]", verb);
    const auto api_json_help
        = fmt::format("OpenVR API JSON definition file [\"{:s}\"]", api_json);
    const auto anon_help
        = fmt::format("anonymize serial numbers in the output [{}]", anon);

    // Use this construct to accept an "empty" command. First parse all
    // together (cli_cmds, cli_opts) then cli_opts to accept also only the
    // options without any command.
    const auto cli_opts
        = ((option("-a", "--api_json") & value("name", api_json)) % api_json_help,
           (option("-o", "--out_json") & value("name", out_json)) % "JSON output file",
           (option("-v", "--verb").set(verb, 1) & opt_value("level", verb)) % verb_help,
           (option("-n", "--anonymize").set(anon, !anon)) % anon_help);

    const auto cli_nocmd = cli_opts;
    const auto cli_cmds
        = ((command("geom").set(selected, mode::geom).doc("show only geometry data")
                | command("props")
                      .set(selected, mode::props)
                      .doc("show only device properties")
                | command("all")
                      .set(selected, mode::all)
                      .doc("show all data (default choice)"),
            cli_nocmd)
           | command("version")
                 .set(selected, mode::info)
                 .doc("show version and other info")
           | command("help").set(selected, mode::help).doc("show this help page"));

    const auto cli = cli_cmds;
    const auto cli_dup = (cli_cmds | cli_nocmd);

    // build C-like argument array from UTF-8 arguments
    auto [cargv, buff] = get_c_argv(u8args);
    int res = 0;
    if (parse(cargv->size(), &(*cargv)[0], cli_dup)) {
        switch (selected) {
            case mode::info:
                print_info(ind, ts);
                break;
            case mode::geom:
            case mode::props:
            case mode::all:
                res = run_wrapper(selected, std::filesystem::path(api_json),
                                  std::filesystem::path(out_json), anon, verb, ind, ts);
                break;
            case mode::help:
                fmt::print("Usage:\n{:s}\nOptions:\n{:s}\n",
                           usage_lines(cli, PROG_NAME).str(), documentation(cli).str());
                break;
        }
    }
    else {
        if (parse(cargv->size(), reinterpret_cast<char**>(&(*cargv)[0]), cli_nocmd)) {
            res = run_wrapper(mode::all, api_json, out_json, anon, verb, ind, ts);
        }
        else {
            fmt::print("Usage:\n{:s}\n", usage_lines(cli, PROG_NAME).str());
            res = 1;
        }
    }
    return res;
}
