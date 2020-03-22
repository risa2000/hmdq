/******************************************************************************
 * HMDQ Tools - tools for an OpenVR HMD and other hardware introspection      *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2020, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#include <filesystem>
//#include <fstream>

#include <fmt/format.h>

#include <xtensor/xjson.hpp>

#include "base_common.h"
#include "calcview.h"
#include "config.h"
#include "fmthlp.h"
#include "jkeys.h"
#include "jtools.h"
#include "oculus_processor.h"
#include "oculus_props.h"
#include "prtdata.h"
#include "xtdef.h"

namespace oculus {

//  constants
//------------------------------------------------------------------------------
//  use instead of valid property ID, makes PID not printed
constexpr int PROP_ID_UNKNOWN = -1;

//  locals
//------------------------------------------------------------------------------
//  properties to hash for PROPS_TO_HASH to "seed" (differentiate) same S/N from
//  different manufacturers (in this order)
const std::vector<std::string> PROPS_TO_SEED
    = {Prop::Manufacturer_String, Prop::ProductName_String};

//  print functions (miscellanous)
//------------------------------------------------------------------------------
void print_oculus(const json& jd, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();
    if (verb >= vdef) {
        //        iprint(sf, "Oculus runtime: {:s}\n", jd[j_rt_path].get<std::string>());
        iprint(sf, "Oculus version: {:s}\n", jd[j_rt_ver].get<std::string>());
    }
}

//  Print enumerated devices.
void print_devs(const json& devs, int ind, int ts)
{
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();
    const auto sf = ind * ts;
    const auto sf1 = (ind + 1) * ts;

    iprint(sf, "Device enumeration:\n");
    /*
    hdevlist_t res;
    for (const auto [dev_id, dev_class] : devs.get<hdevlist_t>()) {
        const auto cname = api[j_classes][std::to_string(dev_class)].get<std::string>();
        iprint(sf1, "Found dev: id={:d}, class={:d}, name={:s}\n", dev_id, dev_class,
               cname);
    }
    */
}

//  Print device properties.
void print_dev_props(const json& api, const json& dprops, int verb, int ind, int ts)
{
    const auto verb_props = g_cfg[j_oculus][j_verbosity][j_properties];

    for (const auto& [pname, pval] : dprops.items()) {
        basevr::print_one_prop(pname, pval, PROP_ID_UNKNOWN, verb_props, verb, ind, ts);
    }
}

//  Print all properties for all devices.
void print_all_props(const json& props, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();

    /*
    for (const auto& [sdid, dprops] : props.items()) {
        const auto dclass
            = dprops["Prop_DeviceClass_Int32"].get<vr::ETrackedDeviceClass>();
        const auto dcname = api[j_classes][std::to_string(dclass)].get<std::string>();
        if (verb >= vdef) {
            iprint(sf, "[{:s}:{:s}]\n", sdid, dcname);
        }
        print_dev_props(api, dprops, verb, ind + 1, ts);
    }
    */
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
    if (m_jData.find(j_geometry) != m_jData.end()) {
        m_jData[j_geometry] = calc_geometry(m_jData[j_geometry]);
    }
}

// Anonymize sensitive data
void Processor::anonymize()
{
    if (m_jData.find(j_properties) != m_jData.end()) {
        const std::vector<std::string> anon_props_names
            = g_cfg[j_oculus][j_anonymize][j_properties].get<std::vector<std::string>>();
        for (auto& [dev, jdprops] : m_jData[j_properties].items()) {
            anonymize_jdprops(jdprops, anon_props_names, PROPS_TO_SEED);
        }
    }
}

// Print the collected data
// mode: props, geom, all
// verb: verbosity
// ind: indentation
// ts: indent (tab) size
void Processor::print(pmode mode, int verb, int ind, int ts)
{
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();
    const auto vsil = g_cfg[j_verbosity][j_silent].get<int>();

    // if there was an error and there are no data, print the error and quit
    if (m_jData.find(ERROR_PREFIX) != m_jData.end()) {
        if (verb >= vdef) {
            iprint(ind * ts, ERR_MSG_FMT_OUT, m_jData[ERROR_PREFIX]);
            // fmt::print("\n");
        }
        return;
    }

    print_oculus(m_jData, verb, ind, ts);
    if (verb >= vdef)
        fmt::print("\n");

    // print the devices and the properties
    auto tverb = (mode == pmode::props || mode == pmode::all) ? verb : vsil;
    if (tverb >= vdef) {
        if (m_jData.find(j_devices) != m_jData.end()) {
            print_devs(m_jData[j_devices], ind, ts);
            fmt::print("\n");
        }
        if (m_jData.find(j_properties) != m_jData.end()) {
            print_all_props(m_jData[j_properties], tverb, ind, ts);
            fmt::print("\n");
        }
    }

    // print all the geometry
    tverb = (mode == pmode::geom || mode == pmode::all) ? verb : vsil;
    if (tverb >= vdef) {
        if (m_jData.find(j_geometry) != m_jData.end()) {
            print_geometry(m_jData[j_geometry], tverb, ind, ts);
            // fmt::print("\n");
        }
    }
}

// Clean up the data before saving
void Processor::purge()
{
    if (m_jData.find(j_properties) != m_jData.end()) {
        for (auto& [sdid, dprops] : m_jData[j_properties].items()) {
            // purge_jdprops_errors(dprops);
        }
    }
}

// Return OculusVR subystem ID
std::string Processor::get_id()
{
    return j_oculus;
}

// Return OculusVR data
json& Processor::get_data()
{
    return m_jData;
}

} // namespace oculus
