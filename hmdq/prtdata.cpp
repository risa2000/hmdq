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

#include <string>
#include <vector>

#include "prtstl.h"
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <openvr/openvr.h>

#include <xtensor/xjson.hpp>

#include "calcview.h"
#include "config.h"
#include "except.h"
#include "fmthlp.h"
#include "jkeys.h"
#include "misc.h"
#include "prtdata.h"
#include "xtdef.h"

#include "fifo_map_fix.h"

//  locals
//------------------------------------------------------------------------------
static constexpr const char* DEG = "deg";
static constexpr const char* MM = "mm";
static constexpr const char* PRCT = "%";
static constexpr auto MM_IN_METER = 1000;

//  functions (miscellanous)
//------------------------------------------------------------------------------
//  Print header (displayed when the execution starts) needs verbosity=silent
void print_header(const char* prog_name, const char* prog_ver, const char* prog_desc,
                  int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto vsil = g_cfg[j_verbosity][j_silent].get<int>();
    if (verb >= vsil) {
        iprint(sf, "{:s} version {:s} - {:s}\n", prog_name, prog_ver, prog_desc);
        fmt::print("\n");
    }
}

//  Print miscellanous info.
void print_misc(const json& jd, const char* prog_name, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();
    if (verb >= vdef) {
        const std::vector<std::pair<std::string, std::string>> msg_templ = {
            {"Time stamp", jd[j_time].get<std::string>()},
            {fmt::format("{:s} version", prog_name), jd[j_hmdq_ver].get<std::string>()},
            {"Output version", std::to_string(jd[j_log_ver].get<int>())},
            {"OS version", jd[j_os_ver].get<std::string>()}};

        size_t maxlen = 0;
        for (const auto& line : msg_templ) {
            const auto tlen = line.first.length();
            maxlen = (tlen > maxlen) ? tlen : maxlen;
        }
        // now print the lines
        for (const auto& line : msg_templ) {
            iprint(sf, "{:>{}s}: {:s}\n", line.first, maxlen, line.second);
        }
    }
}

//  functions (geometry)
//------------------------------------------------------------------------------
//  Print out the raw (tangent) LRBT values.
void print_raw_lrbt(const json& jd, int ind, int ts)
{
    const auto sf = ind * ts;
    constexpr auto s1 = 8; // strlen("bottom: ");
    iprint(sf, "{:{}s}{:14.6f}\n", "left:", s1, jd[j_tan_left].get<double>());
    iprint(sf, "{:{}s}{:14.6f}\n", "right:", s1, jd[j_tan_right].get<double>());
    iprint(sf, "{:{}s}{:14.6f}\n", "bottom:", s1, jd[j_tan_bottom].get<double>());
    iprint(sf, "{:{}s}{:14.6f}\n", "top:", s1, jd[j_tan_top].get<double>());
}

//  Print single eye FOV values in degrees.
void print_fov(const json& jd, int ind, int ts)
{
    const auto sf = ind * ts;
    constexpr auto s1 = 8; // strlen("bottom: ");
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "left:", s1, jd[j_deg_left].get<double>(), DEG);
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "right:", s1, jd[j_deg_right].get<double>(), DEG);
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "bottom:", s1, jd[j_deg_bottom].get<double>(),
           DEG);
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "top:", s1, jd[j_deg_top].get<double>(), DEG);
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "horiz.:", s1, jd[j_deg_hor].get<double>(), DEG);
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "vert.:", s1, jd[j_deg_ver].get<double>(), DEG);
}

//  Print total stereo FOV values in degrees.
void print_fov_total(const json& jd, int ind, int ts)
{
    const auto sf = ind * ts;
    constexpr auto s1 = 12; // strlen("horizontal: ");
    iprint(sf, "{:{}s}{:6.2f} {:s}\n", "horizontal:", s1, jd[j_fov_hor].get<double>(),
           DEG);
    iprint(sf, "{:{}s}{:6.2f} {:s}\n", "vertical:", s1, jd[j_fov_ver].get<double>(), DEG);
    iprint(sf, "{:{}s}{:6.2f} {:s}\n", "diagonal:", s1, jd[j_fov_diag].get<double>(),
           DEG);
    iprint(sf, "{:{}s}{:6.2f} {:s}\n", "overlap:", s1, jd[j_overlap].get<double>(), DEG);
}

//  Print view geometry (panel rotation, IPD).
void print_view_geom(const json& jd, int ind, int ts)
{
    const auto sf = ind * ts;
    constexpr auto s1 = 22; // strlen("right panel rotation: ");
    iprint(sf, "{:{}s}{:6.1f} {:s}\n", "left panel rotation:", s1,
           jd[j_left_rot].get<double>(), DEG);
    iprint(sf, "{:{}s}{:6.1f} {:s}\n", "right panel rotation:", s1,
           jd[j_right_rot].get<double>(), DEG);
    const auto ipd = jd[j_ipd].get<double>() * MM_IN_METER;
    iprint(sf, "{:{}s}{:6.1f} {:s}\n", "reported IPD:", s1, ipd, MM);
}

//  Print the hidden area mask mesh statistics.
void print_ham_mesh(const json& ham_mesh, int verb, int vgeom, int ind, int ts)
{
    const auto sf = ind * ts;
    constexpr auto s1 = 18; // strlen("optimized vertices");

    if (ham_mesh.is_null()) {
        iprint(sf, "No mesh defined by the headset\n");
        return;
    }
    const auto nverts = ham_mesh[j_verts_raw].size();
    // just a safety check that the data are authentic
    HMDQ_ASSERT(nverts % 3 == 0);
    const auto nfaces = nverts / 3;
    const auto nverts_opt = ham_mesh[j_verts_opt].size();
    const auto nfaces_opt = ham_mesh[j_faces_opt].size();
    const auto ham_area = ham_mesh[j_ham_area].get<double>();

    iprint(sf, "{:>{}s}: {:d}, triangles: {:d}\n", "original vertices", s1, nverts,
           nfaces);
    if (verb >= vgeom) {
        iprint(sf, "{:>{}s}: {:d}, n-gons: {:d}\n", "optimized vertices", s1, nverts_opt,
               nfaces_opt);
    }
    iprint(sf, "{:>{}s}: {:.2f} {:s}\n", "mesh area", s1, ham_area * 100, PRCT);
}

//  Print all the info about the view geometry, calculated FOVs, hidden area mesh, etc.
void print_geometry(const json& jd, int verb, int ind, int ts)
{
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();
    const auto vgeom = g_cfg[j_verbosity][j_geometry].get<int>();
    const auto sf = ind * ts;

    if (verb < vdef) {
        return;
    }
    if (jd.find(j_rec_rts) != jd.end()) {
        const auto rec_rts = jd[j_rec_rts].get<std::vector<uint32_t>>();
        iprint(sf, "Recommended render target size: {}\n\n", rec_rts);
    }
    for (const auto& neye : {j_leye, j_reye}) {

        if (jd.find(j_ham_mesh) != jd.end()) {
            iprint(sf, "{:s} eye HAM mesh:\n", neye);
            print_ham_mesh(jd[j_ham_mesh][neye], verb, vgeom, ind + 1, ts);
            fmt::print("\n");
        }
        if (verb >= vgeom) {
            if (jd.find(j_eye2head) != jd.end()) {
                const harray2d_t e2h = jd[j_eye2head][neye];
                iprint(sf, "{:s} eye to head transformation matrix:\n", neye);
                print_harray(e2h, ind + 1, ts);
                fmt::print("\n");
            }

            if (jd.find(j_raw_eye) != jd.end()) {
                iprint(sf, "{:s} eye raw LRBT values:\n", neye);
                print_raw_lrbt(jd[j_raw_eye][neye], ind + 1, ts);
                fmt::print("\n");
            }
        }
        // print eye FOV points only if eye FOV is different from head FOV
        if (jd.find(j_fov_eye) != jd.end() && !jd[j_fov_eye].is_null()) {
            iprint(sf, "{:s} eye raw FOV:\n", neye);
            print_fov(jd[j_fov_eye][neye], ind + 1, ts);
            fmt::print("\n");
        }
        if (jd.find(j_fov_head) != jd.end()) {
            iprint(sf, "{:s} eye head FOV:\n", neye);
            print_fov(jd[j_fov_head][neye], ind + 1, ts);
            fmt::print("\n");
        }
    }
    if (jd.find(j_fov_tot) != jd.end()) {
        iprint(sf, "Total FOV:\n");
        print_fov_total(jd[j_fov_tot], ind + 1, ts);
        fmt::print("\n");
    }
    if (jd.find(j_view_geom) != jd.end()) {
        iprint(sf, "View geometry:\n");
        print_view_geom(jd[j_view_geom], ind + 1, ts);
    }
}

bool have_sensible_data(const json& jd)
{
    if (jd.empty() || jd.is_null()) {
        return false;
    }
    if (jd.find(ERROR_PREFIX) != jd.cend()) {
        return false;
    }
    return true;
}

//  functions (all print)
//------------------------------------------------------------------------------
//  Print the complete data file.
void print_all(const pmode selected, const json& out, const procmap_t& processors,
               int verb, int ind, int ts)
{
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();
    const auto vsil = g_cfg[j_verbosity][j_silent].get<int>();
    const auto verr = g_cfg[j_verbosity][j_error].get<int>();
    const auto sf = ind * ts;
    const auto log_ver = out[j_misc][j_log_ver].get<int>();

    // print the miscellanous (system and app) data
    if (verb >= vdef) {
        print_misc(out[j_misc], PROG_NAME, verb, ind, ts);
        fmt::print("\n");
        // print all the VR from different processors
        bool printed = false;
        for (const auto& [proc_id, proc] : processors) {
            auto pjdata = proc->get_data();
            if (have_sensible_data(*pjdata) || verb >= verr) {
                iprint(sf, "... Subsystem: {} ...\n", proc->get_id());
                fmt::print("\n");
                proc->print(selected, verb, ind, ts);
                fmt::print("\n");
                printed = true;
            }
        }
        if (!printed) {
            iprint(sf, "... No active VR subsystem found ...\n");
        }
    }
}
