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

#include "calcview.h"
#include "except.h"
#include "jkeys.h"
#include "jtools.h"
#include "misc.h"
#include "verhlp.h"

#include "json_proxy.h"

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <iomanip>
#include <string>

//  locals
//------------------------------------------------------------------------------
//  miscellanous section keys
static constexpr auto MM_IN_METER = 1000;
// static constexpr double HAM_AREA_ROUNDOFF = 0.00001;
static constexpr double HAM_AREA_ROUNDOFF = std::numeric_limits<double>::epsilon();

//  fix identifications
//------------------------------------------------------------------------------
static constexpr const char* PROG_VER_DATETIME_FORMAT_FIX = "0.3.1";
static constexpr const char* PROG_VER_OPENVR_SECTION_FIX = "1.0.0";
static constexpr const char* PROG_VER_IPD_FIX = "1.2.3";
static constexpr const char* PROG_VER_FOV_FIX = "1.2.4";
static constexpr const char* PROG_VER_OPENVR_LOCALIZED = "1.3.4";
static constexpr const char* PROG_VER_TRIS_OPT_TO_FACES_RAW = "1.3.91";
static constexpr const char* PROG_VER_NEW_FOV_ALGO = "2.1.0";
static constexpr const char* PROG_VER_NEW_HAM_ALGO = "2.2.0";

//  functions
//------------------------------------------------------------------------------
//  Return 'hmdv_ver' if it is defined in JSON data, otherwise return 'hmdq_ver'.
std::string get_hmdx_ver(const json& jd)
{
    const auto misc = jd[j_misc];
    std::string hmdx_ver;
    if (misc.contains(j_hmdv_ver)) {
        hmdx_ver = misc[j_hmdv_ver].get<std::string>();
    }
    else {
        hmdx_ver = misc[j_hmdq_ver].get<std::string>();
    }
    return hmdx_ver;
}

//  Fix datetime format in misc.time field (ver < v0.3.1)
void fix_datetime_format(json& jd)
{
    std::istringstream stime;
    std::tm tm = {};
    // get the old format with 'T' in the middle
    stime.str(jd[j_misc][j_time].get<std::string>());
    stime >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    // put the new format without 'T'
    jd[j_misc][j_time] = fmt::format("{:%F %T}", tm);
}

//  Fix for moved OpenVR things from 'misc' to 'openvr'
void fix_misc_to_openvr(json& jd)
{
    json jopenvr;
    if (jd[j_misc].contains("openvr_ver")) {
        jopenvr[j_rt_ver] = jd[j_misc]["openvr_ver"];
        jd[j_misc].erase("openvr_ver");
    }
    else {
        jopenvr[j_rt_ver] = "n/a";
    }
    jopenvr[j_rt_path] = "n/a";
    jd[j_openvr] = jopenvr;
}

void fix_ipd_unit(json& jd)
{
    // the old IPD is in milimeters, transform it to meters
    const auto ipd = jd[j_geometry][j_view_geom][j_ipd].get<double>() / MM_IN_METER;
    jd[j_geometry][j_view_geom][j_ipd] = ipd;
}

//  Fix for vertical FOV calculation in (ver < v1.2.4)
void fix_fov_calc(json& jd)
{
    const auto fov_tot = calc_total_fov(jd[j_geometry][j_fov_head]);
    jd[j_geometry][j_fov_tot] = fov_tot;
}

//  move openvr data into openvr section (ver < v1.3.4)
void fix_openvr_section(json& jd)
{
    jd[j_openvr][j_devices] = jd[j_devices];
    jd[j_openvr][j_properties] = jd[j_properties];
    jd[j_openvr][j_geometry] = jd[j_geometry];
    jd.erase(j_devices);
    jd.erase(j_properties);
    jd.erase(j_geometry);
}

//  collect valid geom sections (can be multiple if both runtimes were running)
std::vector<json*> collect_geoms(json& jd)
{
    std::vector<json*> geoms;
    if (jd.contains(j_openvr)) {
        json& jd_openvr = jd[j_openvr];
        if (jd_openvr.contains(j_geometry) && !has_error(jd_openvr[j_geometry])) {
            geoms.push_back(&jd_openvr[j_geometry]);
        }
    }
    if (jd.contains(j_oculus)) {
        json& jd_oculus = jd[j_oculus];
        if (jd_oculus.contains(j_geometry)) {
            for (const auto& fov_id : {j_default_fov, j_max_fov}) {
                if (jd_oculus[j_geometry].contains(fov_id)
                    && !has_error(jd_oculus[j_geometry][fov_id])) {
                    geoms.push_back(&jd_oculus[j_geometry][fov_id]);
                }
            }
        }
    }
    return geoms;
}

//  change (temporarily introduced) 'tris_opt' key to 'faces_raw' and make 'verts_opt'
//  without 'verts_raw' 'verts_raw' again
void fix_tris_opt(json& jd)
{
    auto geoms = collect_geoms(jd);
    for (json* g : geoms) {
        if ((*g).contains(j_ham_mesh)) {
            for (const auto& neye : {j_leye, j_reye}) {
                if ((*g)[j_ham_mesh].contains(neye)
                    && !(*g)[j_ham_mesh][neye].is_null()) {
                    json& ham_eye = (*g)[j_ham_mesh][neye];
                    bool recalc{};
                    if (ham_eye.contains(j_tris_opt)) {
                        ham_eye[j_faces_raw] = std::move(ham_eye[j_tris_opt]);
                        ham_eye.erase(j_tris_opt);
                        recalc = recalc || true;
                    }
                    if (ham_eye.contains(j_verts_opt) && !ham_eye.contains(j_verts_raw)) {
                        ham_eye[j_verts_raw] = std::move(ham_eye[j_verts_opt]);
                        ham_eye.erase(j_verts_opt);
                        recalc = recalc || true;
                    }
                    // calculate optimized HAM mesh values
                    if (recalc) {
                        (*g)[j_ham_mesh][neye] = calc_opt_ham_mesh(ham_eye);
                    }
                }
            }
        }
    }
}

//  recalculate FOV points with the new algorithm which works fine with total FOV > 180
//  deg.
void fix_fov_algo(json& jd)
{
    auto geoms = collect_geoms(jd);
    for (json* g : geoms) {
        auto recalc_geom = calc_geometry(*g);
        (*g)[j_view_geom] = recalc_geom[j_view_geom];
        (*g)[j_fov_eye] = recalc_geom[j_fov_eye];
        (*g)[j_fov_head] = recalc_geom[j_fov_head];
        (*g)[j_fov_tot] = recalc_geom[j_fov_tot];
    }
}

bool fix_ham_area_algo(json& jd)
{
    auto geoms = collect_geoms(jd);
    bool fixed{};
    for (json* g : geoms) {
        if ((*g).contains(j_ham_mesh)) {
            auto& ham_mesh = (*g)[j_ham_mesh];
            for (const auto& neye : {j_leye, j_reye}) {
                if (!ham_mesh[neye].is_null()) {
                    auto& ham_eye = ham_mesh[neye];
                    auto ham_area = calc_ham_area(ham_eye);
                    if (!ham_eye.contains(j_ham_area)
                        || std::abs(ham_area - ham_eye[j_ham_area].get<double>())
                            >= HAM_AREA_ROUNDOFF) {
                        ham_eye[j_ham_area] = ham_area;
                        fixed = true;
                    }
                }
            }
        }
    }
    return fixed;
}

//  Check and run all fixes (return true if there was any)
bool apply_all_relevant_fixes(json& jd)
{
    HMDQ_ASSERT(jd.contains(j_misc));

    const auto hmdx_ver = get_hmdx_ver(jd);
    bool fixed = false;
    // datetime format fix - remove 'T' in the middle
    if (comp_ver(hmdx_ver, PROG_VER_DATETIME_FORMAT_FIX) < 0) {
        fix_datetime_format(jd);
        fixed = true;
    }
    // moved OpenVR things from 'misc' to 'openvr'
    if (comp_ver(hmdx_ver, PROG_VER_OPENVR_SECTION_FIX) < 0) {
        fix_misc_to_openvr(jd);
        fixed = true;
    }
    // change IPD unit in the JSON file - mm -> meters
    if (comp_ver(hmdx_ver, PROG_VER_IPD_FIX) < 0) {
        fix_ipd_unit(jd);
        fixed = true;
    }
    // change the vertical FOV calculation formula
    if (comp_ver(hmdx_ver, PROG_VER_FOV_FIX) < 0) {
        fix_fov_calc(jd);
        fixed = true;
    }
    // move all OpenVR data into 'openvr' section
    if (comp_ver(hmdx_ver, PROG_VER_OPENVR_LOCALIZED) < 0) {
        fix_openvr_section(jd);
        fixed = true;
    }
    // change (temporarily introduced) 'tris_opt' key to 'faces_raw' and make 'verts_opt'
    // without 'verts_raw' 'verts_raw' again
    if (comp_ver(hmdx_ver, PROG_VER_TRIS_OPT_TO_FACES_RAW) < 0) {
        fix_tris_opt(jd);
        fixed = true;
    }
    //  recalculate FOV points with the new algorithm which works fine with total FOV >
    //  180 deg.
    if (comp_ver(hmdx_ver, PROG_VER_NEW_FOV_ALGO) < 0) {
        fix_fov_algo(jd);
        fixed = true;
    }
    //  recalculate HAM area using a new algo which honors HAM out of (0,0), (1,1)
    //  rectangle.
    if (comp_ver(hmdx_ver, PROG_VER_NEW_HAM_ALGO) < 0) {
        auto local = fix_ham_area_algo(jd);
        fixed = local || fixed;
    }
    // add 'hmdv_ver' into misc, if some change was made
    if (fixed) {
        jd[j_misc][j_hmdv_ver] = PROG_VERSION;
    }
    return fixed;
}
