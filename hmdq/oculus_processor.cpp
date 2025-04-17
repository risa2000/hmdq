/******************************************************************************
 * HMDQ Tools - tools for VR headsets and other hardware introspection        *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2020, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#include "oculus_processor.h"
#include "base_common.h"
#include "calcview.h"
#include "config.h"
#include "fmthlp.h"
#include "jkeys.h"
#include "jtools.h"
#include "oculus_common.h"
#include "oculus_props.h"
#include "prtdata.h"
#include "xtdef.h"

#include <nlohmann/ordered_map.hpp>

#include <Extras/OVR_Math.h>

#include <xtensor/xadapt.hpp>
#include <xtensor/xbuilder.hpp>
#include <xtensor/xjson.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <filesystem>

namespace oculus {

//  constants
//------------------------------------------------------------------------------
//  use instead of valid property ID, makes PID not printed
constexpr int PROP_ID_UNKNOWN = -1;

//  properties to hash for PROPS_TO_HASH to "seed" (differentiate) same S/N from
//  different manufacturers (in this order)
const std::vector<std::string> PROPS_TO_SEED
    = {Prop::Manufacturer_String, Prop::ProductName_String};

//  helper functions
//------------------------------------------------------------------------------
template<typename T, typename S>
std::vector<std::string> bitmap_to_flags(T val, const nlohmann::ordered_map<T, S>& bmap)
{
    std::vector<std::string> res;
    for (const auto [mask, name] : bmap) {
        if (0 != (val & mask)) {
            res.push_back(name);
        }
    }
    return res;
}

std::vector<std::string> get_controller_names(int val)
{
    return bitmap_to_flags(val, g_bmControllerTypes);
}

//  print functions (miscellanous)
//------------------------------------------------------------------------------
void print_oculus(const json& jd, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();
    if (verb >= vdef) {
        iprint(sf, "Oculus runtime version: {:s}\n", jd[j_rt_ver].get<std::string>());
    }
}

//  Print enumerated devices.
void print_devs(const json& devs, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto sf1 = (ind + 1) * ts;

    iprint(sf, "Device enumeration:\n");
    iprint(sf1, "HMD: {}\n", devs.at(j_hmd).get<uint32_t>() ? "present" : "absent");
    iprint(sf1, "Tracker count: {}\n", devs.at(j_trackers).get<uint32_t>());
    auto ctrl_types = devs.at(j_ctrl_types).get<uint32_t>();
    const auto ctrl_names = get_controller_names(ctrl_types);
    const auto ctrl_list
        = fmt::format("{}", fmt::join(ctrl_names.cbegin(), ctrl_names.cend(), ", "));
    iprint(sf1, "Controller types: {:#010x}, [{}]\n", ctrl_types, ctrl_list);
}

//  Print device properties.
void print_dev_props(const json& dprops, int verb, int ind, int ts)
{
    const auto verb_props = g_cfg[j_oculus][j_verbosity][j_properties];

    int propId = 1;
    for (const auto& [pname, pval] : dprops.items()) {
        basevr::print_one_prop(pname, pval, propId++, verb_props, verb, ind, ts);
    }
}

//  Print all properties for all devices.
void print_all_props(const json& props, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();

    for (const auto& [sdev, dprops] : props.items()) {
        if (verb >= vdef) {
            iprint(sf, "[{:s}]\n", sdev);
        }
        print_dev_props(dprops, verb, ind + 1, ts);
    }
}

//  Precalculate some data which are not directly provided by Oculus runtime.
void precalc_geometry(json& jd)
{
    json eye2head;

    for (const auto& neye : {j_leye, j_reye}) {

        const ovrPosef jpose = jd[j_render_desc][neye][j_hmd2eye_pose].get<ovrPosef>();
        const OVR::Pose<float> pose(jpose.Orientation, jpose.Position);
        const OVR::Matrix4<float> trans(pose);
        const auto e2h = xt::adapt(&trans.M[0][0], {3, 4});
        eye2head[neye] = e2h;
    }

    // add this to the end of the JSON object
    jd[j_eye2head] = eye2head;
}

//  OculusVR Processor class
//------------------------------------------------------------------------------
// Initialize the processor
bool Processor::init()
{
    return true;
}

// Calculate the complementary data
void Processor::calculate()
{
    if (m_pjData->find(j_geometry) != m_pjData->end()) {
        auto& geom = (*m_pjData)[j_geometry];
        for (auto& [fovType, fovGeom] : geom.items()) {
            if (geometry_sanity_check(fovGeom)) {
                precalc_geometry(fovGeom);
                geom[fovType] = calc_geometry(fovGeom);
            }
            else {
                add_error(fovGeom, "Geometry data are invalid (check JSON output file)");
            }
        }
    }
}

// Anonymize sensitive data
void Processor::anonymize()
{
    if (m_pjData->find(j_properties) != m_pjData->end()) {
        const std::vector<std::string> anon_props_names
            = g_cfg[j_oculus][j_anonymize][j_properties].get<std::vector<std::string>>();
        for (auto& [sdev, jdprops] : (*m_pjData)[j_properties].items()) {
            anonymize_jdprops(jdprops, anon_props_names, PROPS_TO_SEED);
        }
    }
}

// Print the collected data
// mode: props, geom, all
// verb: verbosity
// ind: indentation
// ts: indent (tab) size
void Processor::print(const print_options& opts, int ind, int ts) const
{
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();
    const auto vsil = g_cfg[j_verbosity][j_silent].get<int>();

    // if there was an error and there are no data, print the error and quit
    if (has_error(*m_pjData)) {
        if (opts.verbosity >= vdef) {
            iprint(ind * ts, ERR_MSG_FMT_OUT, get_error_msg(*m_pjData));
        }
        return;
    }

    print_oculus(*m_pjData, opts.verbosity, ind, ts);
    if (opts.verbosity >= vdef)
        fmt::print("\n");

    // print the devices and the properties
    auto tverb
        = (opts.mode == pmode::props || opts.mode == pmode::all) ? opts.verbosity : vsil;
    if (tverb >= vdef) {
        if (m_pjData->find(j_devices) != m_pjData->end()) {
            print_devs((*m_pjData)[j_devices], ind, ts);
            fmt::print("\n");
        }
        if (m_pjData->find(j_properties) != m_pjData->end()) {
            print_all_props((*m_pjData)[j_properties], tverb, ind, ts);
            fmt::print("\n");
        }
    }

    // print all the geometry
    tverb = (opts.mode == pmode::geom || opts.mode == pmode::all) ? opts.verbosity : vsil;
    if (tverb >= vdef) {
        if (m_pjData->find(j_geometry) != m_pjData->end()) {
            auto& geom = (*m_pjData)[j_geometry];
            const auto sf = ind * ts;
            bool print_nl = false;
            for (auto& [fovType, fovGeom] : geom.items()) {
                if (fovType == j_max_fov && opts.ovr_max_fov
                    || fovType == j_default_fov && opts.ovr_def_fov) {
                    //  print the new line in between the different FOVs, but not before
                    //  (or after)
                    if (print_nl) {
                        fmt::print("\n");
                    }
                    else {
                        print_nl = true;
                    }
                    iprint(sf, "{}:\n", get_jkey_pretty(fovType));
                    fmt::print("\n");
                    if (has_error(fovGeom)) {
                        iprint((ind + 1) * ts, ERR_MSG_FMT_OUT, get_error_msg(fovGeom));
                    }
                    else {
                        print_geometry(fovGeom, tverb, ind + 1, ts);
                    }
                }
            }
        }
    }
}

// Clean up the data before saving
void Processor::purge()
{
    if (m_pjData->find(j_properties) != m_pjData->end()) {
        auto& props = (*m_pjData)[j_properties];
        for (auto& [sdid, dprops] : props.items()) {
            purge_jdprops_errors(dprops);
        }
    }
}

} // namespace oculus
