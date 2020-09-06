/******************************************************************************
 * HMDQ Tools - tools for VR headsets and other hardware introspection        *
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

#include <clipp.h>

#include <botan/version.h>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <Eigen/Core>

#include <xtensor/xtensor_config.hpp>
#include <xtl/xtl_config.hpp>

#include "compat.h"
#include "config.h"
#include "except.h"
#include "fmthlp.h"
#include "gitversion.h"
#include "hmdfix.h"
#include "jkeys.h"
#include "json_proxy.h"
#include "jtools.h"
#include "misc.h"
#include "oculus_config.h"
#include "oculus_processor.h"
#include "openvr_common.h"
#include "openvr_config.h"
#include "openvr_processor.h"
#include "prtdata.h"
#include "wintools.h"

//  defines
//------------------------------------------------------------------------------
#define xstringify(_s) stringify(_s)
#define stringify(_x) #_x

//  typedefs
//------------------------------------------------------------------------------
//  mode of operation
enum class mode { geom, props, all, verify, info, help };

//  locals
//------------------------------------------------------------------------------
static constexpr int IND = 0;
static constexpr unsigned int CP_UTF8 = 65001;
const auto OPENVR_API_JSON = "openvr_api.json";

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
    iprint(sf1, "{:>{}s}: {:s}\n", "date", tf1, BUILD_TIMESTAMP);
    fmt::print("\n");
    iprint(sf, "using libraries:\n");
    constexpr const char* libver_nover_fmt = "{0} (https://github.com/{0})\n";
    constexpr const char* libver_str_fmt = "{0} {1} (https://github.com/{0})\n";
    constexpr const char* libver_num_fmt = "{0} {1}.{2}.{3} (https://github.com/{0})\n";
    constexpr const char* gitlab_libver_num_fmt
        = "{0} {1}.{2}.{3} (https://gitlab.com/{0})\n";
    iprint(sf1, libver_nover_fmt, "muellan/clip");
    iprint(sf1, libver_num_fmt, "nlohmann/json", NLOHMANN_JSON_VERSION_MAJOR,
           NLOHMANN_JSON_VERSION_MINOR, NLOHMANN_JSON_VERSION_PATCH);
    iprint(sf1, libver_num_fmt, "QuantStack/xtl", XTL_VERSION_MAJOR, XTL_VERSION_MINOR,
           XTL_VERSION_PATCH);
    iprint(sf1, libver_num_fmt, "QuantStack/xtensor", XTENSOR_VERSION_MAJOR,
           XTENSOR_VERSION_MINOR, xstringify(XTENSOR_VERSION_PATCH));
    iprint(sf1, libver_str_fmt, "randombit/botan", Botan::short_version_string());
    iprint(sf1, libver_num_fmt, "fmtlib/fmt", FMT_VERSION / 10000,
           (FMT_VERSION % 10000) / 100, FMT_VERSION % 100);
    iprint(sf1, gitlab_libver_num_fmt, "libeigen/eigen", EIGEN_WORLD_VERSION,
           EIGEN_MAJOR_VERSION, EIGEN_MINOR_VERSION);
}

// Translate the tool selected mode into print mode.
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

//  wrapper for main runner to deal with domestic exceptions
int run_verify(const std::filesystem::path& in_json, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();

    // print the execution header
    print_header(PROG_NAME, PROG_VERSION, PROG_DESCRIPTION, verb, ind, ts);
    if (verb >= vdef)
        fmt::print("\n");

    // read JSON data input
    json out = read_json(in_json);

    // verify the checksum
    auto check_ok = verify_checksum(out);
    if (verb >= vdef) {
        if (check_ok) {
            iprint(sf, "Input file checksum is OK\n");
        }
        else {
            iprint(sf, "Input file checksum is invalid\n");
        }
    }
    return check_ok ? 0 : 1;
}

//  main runner
int run(const print_options& opts, const std::filesystem::path& api_json,
        const std::filesystem::path& in_json, const std::filesystem::path& out_json,
        int ind, int ts)
{
    const auto sf = ind * ts;
    // initialize config values
    const auto json_indent = g_cfg[j_format][j_json_indent].get<int>();
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();

    // print the execution header
    print_header(PROG_NAME, PROG_VERSION, PROG_DESCRIPTION, opts.verbosity, ind, ts);
    if (opts.verbosity >= vdef)
        fmt::print("\n");

    // read JSON data input
    json out = read_json(in_json);

    // verify the checksum
    auto check_ok = verify_checksum(out);
    if (!check_ok && opts.verbosity >= vdef) {
        iprint(sf, "Warning: Input file checksum is invalid\n\n");
    }

    // apply all known fixes
    apply_all_relevant_fixes(out);

    // processor buffer
    procmap_t processors;

    // process all VR subsystem interfaces
    if (out.contains(j_openvr)) {
        auto openvr_processor
            = new openvr::Processor(api_json, std::make_shared<json>(out[j_openvr]));
        openvr_processor->init();
        processors.emplace(openvr_processor->get_id(), openvr_processor);
    }
    if (out.contains(j_oculus)) {
        auto oculus_processor
            = new oculus::Processor(std::make_shared<json>(out[j_oculus]));
        oculus_processor->init();
        processors.emplace(oculus_processor->get_id(), oculus_processor);
    }

    // anonymize the data if requested
    if (opts.anonymize) {
        for (auto& [proc_id, proc] : processors) {
            proc->anonymize();
        }
    }

    // print all
    print_all(opts, out, processors, ind, ts);

    // dump the data into the optional JSON file
    if (!out_json.empty()) {
        out.erase(j_checksum);
        // add the checksum only if the original file was authentic
        if (check_ok) {
            add_checksum(out);
        }
        // save the JSON file with indentation
        write_json(out_json, out, json_indent);
    }
    return 0;
}

//  wrapper for main runner to deal with domestic exceptions
template<typename F, typename... Args>
int run_wrapper(F func, Args&&... args)
{
    int res = 0;
    try {
        res = func(std::forward<Args>(args)...);
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
    const auto OPENVR_API_JSON = "openvr_api.json";

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

    print_options opts;

    // defaults for the arguments
    opts.verbosity = g_cfg[j_verbosity][j_default].get<int>();
    opts.anonymize = g_cfg[j_control][j_anonymize].get<bool>();

    // default command is 'all'
    mode cmd = mode::all;

    // build relative path to OPENVR_API_JSON file
    std::filesystem::path api_json_path = get_full_prog_path();
    api_json_path.replace_filename(OPENVR_API_JSON);
    auto api_json = u8str2str(api_json_path.u8string());

    std::string out_json;
    std::string in_json;
    // custom help texts
    const auto verb_help = fmt::format("verbosity level [{:d}]", opts.verbosity);
    const auto api_json_help
        = fmt::format("OpenVR API JSON definition file [\"{:s}\"]", api_json);
    const auto anon_help
        = fmt::format("anonymize serial numbers in the output [{}]", opts.anonymize);

    // Use this construct to accept an "empty" command. First parse all together
    // (cli_cmds, cli_args, cli_opts) then (cli_args, cli_opts) to accept also only the
    // options without any command.
    auto cli_args = (value("in_json", in_json) % "input data file");
    auto cli_opts
        = ((option("-a", "--api_json") & value("name", api_json)) % api_json_help,
           (option("-o", "--out_json") & value("name", out_json)) % "JSON output file",
           (option("-v", "--verb").set(opts.verbosity, 1)
            & opt_value("level", opts.verbosity))
               % verb_help,
           (option("-n", "--anonymize").set(opts.anonymize, !opts.anonymize) % anon_help),
           (option("--openvr").set(opts.oculus, false).set(opts.openvr, true)
            % "show only OpenVR data"),
           (option("--oculus").set(opts.openvr, false).set(opts.oculus, true)
            % "show only Oculus data"),
           (option("--ovr_max_fov").set(opts.ovr_max_fov, true)
            % "show also Oculus max FOV data"));

    auto cli_nocmd = (cli_opts, cli_args);
    auto cli_cmds
        = ((command("geom").set(cmd, mode::geom).doc("show only geometry data")
                | command("props")
                      .set(cmd, mode::props)
                      .doc("show only device properties")
                | command("all")
                      .set(cmd, mode::all)
                      .doc("show all data (default choice)"),
            cli_nocmd)
           | (command("verify")
                  .set(cmd, mode::verify)
                  .doc("verify the data file integrity"),
              cli_args)
           | command("version").set(cmd, mode::info).doc("show version and other info")
           | command("help").set(cmd, mode::help).doc("show this help page"));

    auto cli = cli_cmds;
    auto cli_dup = (cli_cmds | cli_nocmd);

    // build C-like argument array from UTF-8 arguments
    auto [cargv, buff] = get_c_argv(u8args);
    int res = 0;
    if (parse(cargv->size(), &(*cargv)[0], cli_dup)) {
        switch (cmd) {
            case mode::info:
                print_info(ind, ts);
                break;
            case mode::geom:
            case mode::props:
            case mode::all:
                opts.mode = mode2pmode(cmd);
                res = run_wrapper(run, opts, std::filesystem::path(api_json),
                                  std::filesystem::path(in_json),
                                  std::filesystem::path(out_json), ind, ts);
                break;
            case mode::verify:
                res = run_wrapper(run_verify, std::filesystem::path(in_json),
                                  opts.verbosity, ind, ts);
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
    return res;
}
