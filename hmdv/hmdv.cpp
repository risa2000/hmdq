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

//  main runner
int run(mode selected, const std::string& api_json, const std::string& in_json,
        const std::string& out_json, bool anon, int verb, int ind, int ts)
{
    // initialize config values
    const auto json_indent = g_cfg["format"]["json_indent"].get<int>();
    const auto app_type = g_cfg["openvr"]["app_type"].get<vr::EVRApplicationType>();
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    const auto vsil = g_cfg["verbosity"]["silent"].get<int>();
    const auto verr = g_cfg["verbosity"]["error"].get<int>();

    // print the execution header
    print_header(PROG_NAME, PROG_VERSION, PROG_DESCRIPTION, verb, ind, ts);
    if (verb >= vdef)
        fmt::print("\n");

    // make sure that OpenVR API file (default, or specified) exists
    std::filesystem::path apath = std::filesystem::u8path(api_json);
    if (!std::filesystem::exists(apath)) {
        auto msg
            = fmt::format("OpenVR API JSON file not found: \"{:s}\"", apath.u8string());
        throw hmdq_error(msg);
    }

    // check the specified input file exists
    std::filesystem::path inpath = std::filesystem::u8path(in_json);
    if (!std::filesystem::exists(inpath)) {
        auto msg = fmt::format("Input file not found: \"{:s}\"", inpath.u8string());
        throw hmdq_error(msg);
    }

    // read JSON API def
    std::ifstream jfa(apath);
    json oapi;
    jfa >> oapi;
    jfa.close();

    // parse the API file to hmdq used json (dict)
    const json api = parse_json_oapi(oapi);

    // read JSON data input
    std::ifstream jin(inpath);
    json out;
    jin >> out;
    jin.close();

    // verify the checksum
    auto check_ok = verify_checksum(out);
    if (!check_ok && verb >= vdef) {
        iprint(ind, "Warning: Input file checksum is invalid\n\n");
    }

    print_misc(out["misc"], "hmdq", verb, ind, ts);
    print_openvr(out["openvr"], verb, ind, ts);
    if (verb >= vdef)
        fmt::print("\n");

    // get all the properties
    auto tverb = (selected == mode::props || selected == mode::all) ? verb : vsil;
    if (tverb >= vdef) {
        print_devs(api, out["devices"], ind, ts);
        fmt::print("\n");
    }

    print_all_props(api, out["properties"], tverb, ind, ts);
    if (tverb >= vdef)
        fmt::print("\n");

    // get all the geometry
    tverb = (selected == mode::geom || selected == mode::all) ? verb : vsil;
    print_geometry(out["geometry"], tverb, ind, ts);
    if (tverb >= vdef)
        fmt::print("\n");

    // dump the data into the optional JSON file
    if (out_json.size()) {
        out.erase(CHECKSUM);
        // add the checksum only if the original file was authentic
        if (check_ok) {
            add_checksum(out);
        }
        // save the JSON with indentation
        std::filesystem::path opath = std::filesystem::u8path(out_json);
        std::ofstream jfo(opath);
        jfo << out.dump(json_indent);
    }
    return 0;
}

//  wrapper for main runner to deal with domestic exceptions
int run_wrapper(mode selected, const std::string& api_json, const std::string& in_json,
                const std::string& out_json, bool anon, int verb, int ind, int ts)
{
    int res = 0;
    try {
        res = run(selected, api_json, in_json, out_json, anon, verb, ind, ts);
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

void print_u8args(std::vector<std::string> u8args)
{
    fmt::print("Command line arguments:\n");
    for (int i = 0, e = u8args.size(); i < e; ++i) {
        fmt::print("{:d}: {}\n", i, u8args[i]);
    }
    fmt::print("\n");
}

int main(int argc, char* argv[])
{
    using namespace clipp;
    const auto OPENVR_API_JSON = "openvr_api.json";

    // Set UTF-8 code page for the console if available
    // if not, print UTF-8 strings to the console anyway.
    set_console_cp(CP_UTF8);

    // get command line args in Windows Unicode and in UTF-8
    const auto u8args = get_u8args();
    if (u8args.size() == 0) {
        throw hmdq_error("Cannot get the command line arguments");
    }
    // print_u8args(u8args);

    // init global config before anything else
    const auto cfg_ok = init_config(u8args[0]);
    if (!cfg_ok)
        return 1;

    const auto ts = g_cfg["format"]["cli_indent"].get<int>();
    const auto ind = IND;

    // defaults for the arguments
    auto verb = g_cfg["verbosity"]["default"].get<int>();
    auto anon = g_cfg["control"]["anonymize"].get<bool>();

    // default command is 'all'
    mode selected = mode::all;

    // build relative path to OPENVR_API_JSON file
    auto api_json_path = std::filesystem::relative(
        std::filesystem::absolute(std::filesystem::u8path(u8args[0])));
    api_json_path.replace_filename(OPENVR_API_JSON);
    auto api_json = api_json_path.u8string();

    std::string out_json;
    std::string in_json;
    // custom help texts
    const auto verb_help = fmt::format("verbosity level [{:d}]", verb);
    const auto api_json_help
        = fmt::format("OpenVR API JSON definition file [\"{:s}\"]", api_json);
    const auto anon_help
        = fmt::format("anonymize serial numbers in the output [{}]", anon);

    // Use this construct to accept an "empty" command. First parse all together
    // (cli_cmds, cli_args, cli_opts) then (cli_args, cli_opts) to accept also only the
    // options without any command.
    auto cli_args = (value("in_json", in_json) % "input data file");
    auto cli_opts
        = ((option("-a", "--api_json") & value("name", api_json)) % api_json_help,
           (option("-o", "--out_json") & value("name", out_json)) % "JSON output file",
           (option("-v", "--verb").set(verb, 1) & opt_value("level", verb)) % verb_help,
           (option("-n", "--anonymize").set(anon, !anon)) % anon_help);
    auto cli_nocmd = (cli_args, cli_opts);
    auto cli_cmds
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

    auto cli = cli_cmds;
    auto cli_dup = (cli_cmds | cli_nocmd);

    // build C-like argument array from UTF-8 arguments
    auto [ptrs, buff] = get_c_argv(u8args);
    std::vector<char*> cargv;
    for (auto& p : ptrs) {
        cargv.push_back(p + &buff[0]);
    }

    int res = 0;
    if (parse(cargv.size(), &cargv[0], cli_dup)) {
        switch (selected) {
            case mode::info:
                print_info(ind, ts);
                break;
            case mode::geom:
            case mode::props:
            case mode::all:
                res = run_wrapper(selected, api_json, in_json, out_json, anon, verb, ind,
                                  ts);
                break;
            case mode::help:
                fmt::print("Usage:\n{:s}\nOptions:\n{:s}\n",
                           usage_lines(cli, PROG_NAME).str(), documentation(cli).str());
                break;
        }
    }
    else {
        fmt::print("Usage:\n{:s}\n", usage_lines(cli, PROG_NAME).str());
        res = 1;
    }
    /*
    else {
        if (parse(cargv.size(), &cargv[0], cli_nocmd)) {
            res = run_wrapper(mode::all, api_json, in_json, out_json, anon, verb, ind,
    ts);
        }
        else {
            fmt::print("Usage:\n{:s}\n", usage_lines(cli, PROG_NAME).str());
            res = 1;
        }
    }
    */
    return res;
}
