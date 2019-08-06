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
#include <string>
#include <vector>

#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include <fmt/format.h>

#include "config.h"
#include "except.h"
#include "fmthlp.h"
#include "jtools.h"
#include "ovrhlp.h"
#include "prtdata.h"
#include "xtdef.h"

#include "fifo_map_fix.h"

//  locals
//------------------------------------------------------------------------------
static constexpr const char* DEG = "deg";
static constexpr const char* MM = "mm";
static constexpr const char* PRCT = "%";

//  functions (miscellanous)
//------------------------------------------------------------------------------
//  Print miscellanous info.
void print_misc(const json& jd, const char* prog_name, const char* prog_desc, int verb,
                int ind, int ts)
{
    const auto sf = ind * ts;
    const auto vsil = g_cfg["verbosity"]["silent"].get<int>();
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    if (verb >= vsil) {
        iprint(sf, "{:s} version {:s} - {:s}\n", prog_name,
               jd["hmdq_ver"].get<std::string>(), prog_desc);
    }
    if (verb >= vdef) {
        iprint(sf, "Current time: {:s}\n", jd["time"].get<std::string>());
        iprint(sf, "OS version: {:s}\n", jd["os_ver"].get<std::string>());
        iprint(sf, "OpenVR version: {:s}\n", jd["openvr_ver"].get<std::string>());
    }
}

//  functions (devices and properties)
//------------------------------------------------------------------------------
//  Print enumerated devices.
void print_devs(const json& api, const json& devs, int ind, int ts)
{
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    const auto sf = ind * ts;
    const auto sf1 = (ind + 1) * ts;

    iprint(sf, "Device enumeration:\n");
    hdevlist_t res;
    for (const auto [dev_id, dev_class] : devs.get<hdevlist_t>()) {
        auto cname = api["classes"][std::to_string(dev_class)].get<std::string>();
        iprint(sf1, "Found dev: id={:d}, class={:d}, name={:s}\n", dev_id, dev_class,
               cname);
    }
}

//  Print (non-error) value of an Array type property.
void print_array_type(const std::string& pname, const json& pval, int ind, int ts)
{
    const auto sf = ind * ts;
    // find the name of the type (before "_Array" suffix)
    auto sarray = pname.rfind('_');
    auto stype = pname.rfind('_', sarray - 1) + 1;
    auto ptype = pname.substr(stype, sarray - stype);
    vr::PropertyTypeTag_t ptag = get_ptag_from_ptype(ptype);

    if (ptag == vr::k_unFloatPropertyTag) {
        print_array(pval.get<hvector_t>(), ind, ts);
    }
    else if (ptag == vr::k_unInt32PropertyTag) {
        print_array(pval.get<xt::xtensor<int32_t, 1>>(), ind, ts);
    }
    else if (ptag == vr::k_unUint64PropertyTag) {
        print_array(pval.get<xt::xtensor<uint64_t, 1>>(), ind, ts);
    }
    else if (ptag == vr::k_unBoolPropertyTag) {
        print_array(pval.get<xt::xtensor<bool, 1>>(), ind, ts);
    }
    else if (ptag == vr::k_unHmdMatrix34PropertyTag) {
        print_array(pval.get<xt::xtensor<double, 3>>(), ind, ts);
    }
    else if (ptag == vr::k_unHmdMatrix44PropertyTag) {
        print_array(pval.get<xt::xtensor<double, 3>>(), ind, ts);
    }
    else if (ptag == vr::k_unHmdVector2PropertyTag) {
        print_array(pval.get<xt::xtensor<double, 2>>(), ind, ts);
    }
    else if (ptag == vr::k_unHmdVector3PropertyTag) {
        print_array(pval.get<xt::xtensor<double, 2>>(), ind, ts);
    }
    else if (ptag == vr::k_unHmdVector4PropertyTag) {
        print_array(pval.get<xt::xtensor<double, 2>>(), ind, ts);
    }
    else {
        auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype);
        iprint(sf, ERR_MSG_FMT, msg);
    }
}

//  Print device properties.
void print_dev_props(const json& api, const json& dprops, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto jverb = g_cfg["verbosity"];
    const auto verr = jverb["error"].get<int>();

    for (const auto [pname, pval] : dprops.items()) {
        // convert string to the correct type
        auto pid = api["props"]["name2id"][pname].get<vr::ETrackedDeviceProperty>();
        // property type (the last part after '_')
        auto ptype = pname.substr(pname.rfind('_') + 1);
        // property verbosity level (if defined) or max
        int pverb;
        // property having an error attached?
        auto nerr = pval.count(ERROR_PREFIX);

        // determine the "active" verbosity level for the current property
        if (jverb["props"].count(pname)) {
            // explicitly defined property is always honored
            pverb = jverb["props"][pname].get<int>();
        }
        else {
            // verbosity is not explicitly defined
            if (nerr) {
                // the value is an error code, so set required verbosity to verr
                pverb = verr;
            }
            else {
                // otherwise set requested verbosity to vmax
                pverb = jverb["max"].get<int>();
            }
        }
        if (verb < pverb) {
            // do not print prop values with higher verbosity than the current one
            continue;
        }

        // print the prop name
        iprint(sf, "{:>4d} : {:s} = ", pid, pname);
        if (nerr) {
            auto msg = pval[ERROR_PREFIX].get<std::string>();
            fmt::print(ERR_MSG_FMT, msg);
        }
        else if (ptype == "Bool") {
            fmt::print("{}\n", pval.get<bool>());
        }
        else if (ptype == "String") {
            fmt::print("\"{:s}\"\n", pval.get<std::string>());
        }
        else if (ptype == "Uint64") {
            fmt::print("{:#x}\n", pval.get<uint64_t>());
        }
        else if (ptype == "Int32") {
            fmt::print("{}\n", pval.get<int32_t>());
        }
        else if (ptype == "Float") {
            fmt::print("{:.6g}\n", pval.get<double>());
        }
        else if (ptype == "Matrix34") {
            fmt::print("\n");
            harray2d_t mat34 = pval;
            print_array(mat34, ind + 1, ts);
        }
        else if (ptype == "Array") {
            fmt::print("\n");
            print_array_type(pname, pval, ind + 1, ts);
        }
        else {
            auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype);
            fmt::print(ERR_MSG_FMT, msg);
        }
    }
}

//  Print all properties for all devices.
void print_all_props(const json& api, const json& props, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();

    for (const auto [sdid, dprops] : props.items()) {
        auto dclass = dprops["Prop_DeviceClass_Int32"].get<vr::ETrackedDeviceClass>();
        auto dcname = api["classes"][std::to_string(dclass)].get<std::string>();
        if (verb >= vdef) {
            iprint(sf, "[{:s}:{:s}]\n", sdid, dcname);
        }
        print_dev_props(api, dprops, verb, ind + 1, ts);
    }
}

//  functions (geometry)
//------------------------------------------------------------------------------
//  Print out the raw (tangent) LRBT values.
void print_raw_lrbt(const json& jd, int ind, int ts)
{
    const auto sf = ind * ts;
    constexpr auto s1 = 8; // strlen("bottom: ");
    iprint(sf, "{:{}s}{:14.6f}\n", "left:", s1, jd["tan_left"].get<double>());
    iprint(sf, "{:{}s}{:14.6f}\n", "right:", s1, jd["tan_right"].get<double>());
    iprint(sf, "{:{}s}{:14.6f}\n", "bottom:", s1, jd["tan_bottom"].get<double>());
    iprint(sf, "{:{}s}{:14.6f}\n", "top:", s1, jd["tan_top"].get<double>());
}

//  Print single eye FOV values in degrees.
void print_fov(const json& jd, int ind, int ts)
{
    const auto sf = ind * ts;
    constexpr auto s1 = 8; // strlen("bottom: ");
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "left:", s1, jd["deg_left"].get<double>(), DEG);
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "right:", s1, jd["deg_right"].get<double>(), DEG);
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "bottom:", s1, jd["deg_bottom"].get<double>(),
           DEG);
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "top:", s1, jd["deg_top"].get<double>(), DEG);
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "horiz.:", s1, jd["deg_hor"].get<double>(), DEG);
    iprint(sf, "{:{}s}{:10.2f} {:s}\n", "vert.:", s1, jd["deg_ver"].get<double>(), DEG);
}

//  Print total stereo FOV values in degrees.
void print_fov_total(const json& jd, int ind, int ts)
{
    const auto sf = ind * ts;
    constexpr auto s1 = 12; // strlen("horizontal: ");
    iprint(sf, "{:{}s}{:6.2f} {:s}\n", "horizontal:", s1, jd["fov_hor"].get<double>(),
           DEG);
    iprint(sf, "{:{}s}{:6.2f} {:s}\n", "vertical:", s1, jd["fov_ver"].get<double>(), DEG);
    iprint(sf, "{:{}s}{:6.2f} {:s}\n", "diagonal:", s1, jd["fov_diag"].get<double>(),
           DEG);
    iprint(sf, "{:{}s}{:6.2f} {:s}\n", "overlap:", s1, jd["overlap"].get<double>(), DEG);
}

//  Print view geometry (panel rotation, IPD).
void print_view_geom(const json& jd, int ind, int ts)
{
    const auto sf = ind * ts;
    constexpr auto s1 = 22; // strlen("right panel rotation: ");
    iprint(sf, "{:{}s}{:6.1f} {:s}\n", "left panel rotation:", s1,
           jd["left_rot"].get<double>(), DEG);
    iprint(sf, "{:{}s}{:6.1f} {:s}\n", "right panel rotation:", s1,
           jd["right_rot"].get<double>(), DEG);
    iprint(sf, "{:{}s}{:6.1f} {:s}\n", "reported IPD:", s1, jd["ipd"].get<double>(), MM);
}

//  Print the hidden area mask mesh statistics.
void print_ham_mesh(const json& ham_mesh, const char* neye, int verb, int vgeom, int ind,
                    int ts)
{
    const auto sf = ind * ts;
    const auto sf1 = (ind + 1) * ts;
    constexpr auto s1 = 18; // strlen("optimized vertices");

    iprint(sf, "{:s} eye HAM mesh:\n", neye);

    if (!ham_mesh.is_null()) {
        auto nverts = ham_mesh["verts_raw"].size();
        // just a safety check that the data are authentic
        HMDQ_ASSERT(nverts % 3 == 0);
        auto nfaces = nverts / 3;
        auto nverts_opt = ham_mesh["verts_opt"].size();
        auto nfaces_opt = ham_mesh["faces_opt"].size();
        auto ham_area = ham_mesh["ham_area"].get<double>();

        iprint(sf1, "{:>{}s}: {:d}, triangles: {:d}\n", "original vertices", s1, nverts,
               nfaces);
        if (verb >= vgeom) {
            iprint(sf1, "{:>{}s}: {:d}, n-gons: {:d}\n", "optimized vertices", s1,
                   nverts_opt, nfaces_opt);
        }
        iprint(sf1, "{:>{}s}: {:.2f} {:s}\n", "mesh area", s1, ham_area * 100, PRCT);
    }
    else {
        iprint(sf1, "No mesh defined by the headset\n");
    }
}

//  Print all the info about the view geometry, calculated FOVs, hidden area mesh, etc.
void print_geometry(const json& jd, int verb, int ind, int ts)
{
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    const auto vgeom = g_cfg["verbosity"]["geom"].get<int>();
    const auto sf = ind * ts;

    auto rec_rts = jd["rec_rts"].get<std::vector<uint32_t>>();
    if (verb >= vdef) {
        iprint(sf, "Recommended render target size: {}\n\n", rec_rts);
    }
    for (auto [eye, neye] : EYES) {

        if (verb >= vdef) {
            print_ham_mesh(jd["ham_mesh"][neye], neye.c_str(), verb, vgeom, ind, ts);
            fmt::print("\n");
        }
        if (verb >= vgeom) {
            harray2d_t e2h = jd["eye2head"][neye];
            iprint(sf, "{:s} eye to head transformation matrix:\n", neye);
            print_array(e2h, ind + 1, ts);
            fmt::print("\n");

            iprint(sf, "{:s} eye raw LRBT values:\n", neye);
            print_raw_lrbt(jd["raw_eye"][neye], ind + 1, ts);
            fmt::print("\n");
        }
        // build eye FOV points only if eye FOV is different from head FOV
        if (verb >= vdef) {
            if (!jd["fov_eye"].is_null()) {
                iprint(sf, "{:s} eye raw FOV:\n", neye);
                print_fov(jd["fov_eye"][neye], ind + 1, ts);
                fmt::print("\n");
            }
            iprint(sf, "{:s} eye head FOV:\n", neye);
            print_fov(jd["fov_head"][neye], ind + 1, ts);
            fmt::print("\n");
        }
    }
    if (verb >= vdef) {
        iprint(sf, "Total FOV:\n");
        print_fov_total(jd["fov_tot"], ind + 1, ts);
        fmt::print("\n");

        iprint(sf, "View geometry:\n");
        print_view_geom(jd["view_geom"], ind + 1, ts);
    }
}
