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
#include <fstream>

#include <fmt/format.h>

#include <xtensor/xjson.hpp>

#include "base_common.h"
#include "calcview.h"
#include "config.h"
#include "fmthlp.h"
#include "jkeys.h"
#include "jtools.h"
#include "openvr_common.h"
#include "openvr_processor.h"
#include "prtdata.h"
#include "xtdef.h"

namespace openvr {

//  locals
//------------------------------------------------------------------------------
//  properties to hash for PROPS_TO_HASH to "seed" (differentiate) same S/N from
//  different manufacturers (in this order)
const hproplist_t PROPS_TO_SEED
    = {vr::Prop_ManufacturerName_String, vr::Prop_ModelNumber_String};

//  helper (local) functions for OpenVR processor
//------------------------------------------------------------------------------
//  Return property name and value for given property ID from device properties.
std::tuple<std::string, std::string>
get_prop_name_val(const json& api, const json& dprops, vr::ETrackedDeviceProperty pid)
{
    const auto cat = static_cast<int>(pid) / 1000;
    const auto pname
        = api[j_properties][std::to_string(cat)][std::to_string(pid)].get<std::string>();
    auto pval = std::string();
    // return empty string if the value does not exist or is not string (i.e. is an error)
    if (dprops.find(pname) != dprops.end() && dprops[pname].is_string())
        pval = dprops[pname].get<std::string>();
    return {pname, pval};
}

//  Remove all properties with errors reported from the dict.
void purge_errors(json& jd)
{
    std::vector<std::string> to_drop;
    for (const auto& [sdid, dprops] : jd.items()) {
        std::vector<std::string> to_drop;
        for (const auto& [pname, pval] : dprops.items()) {
            if (pval.count(ERROR_PREFIX)) {
                to_drop.push_back(pname);
            }
        }
        // purge the props with errors
        for (const auto& pname : to_drop) {
            dprops.erase(pname);
        }
    }
}

//  anonymize functions
//------------------------------------------------------------------------------
//  Anonymize properties for particular tracked device.
void anonymize_dev_props(const json& api, json& dprops)
{
    // get the list of properties to hash from the config file
    const std::vector<std::string> anon_props_names
        = g_cfg[j_openvr][j_anonymize][j_properties].get<std::vector<std::string>>();
    const json& name2id = api[j_properties][j_name2id];
    hproplist_t anon_props_ids;
    for (const auto& name : anon_props_names) {
        if (name2id.find(name) != name2id.end()) {
            anon_props_ids.push_back(name2id[name].get<vr::ETrackedDeviceProperty>());
        }
    }
    const std::string anon_prefix(ANON_PREFIX);
    for (const auto pid : anon_props_ids) {
        const auto [pname, pval] = get_prop_name_val(api, dprops, pid);
        if (pval.size() == 0)
            continue;
        // hash only non-empty strings
        const std::string prefix(pval.c_str(), anon_prefix.size());
        if (prefix != anon_prefix) {
            std::vector<char> msgbuff;
            for (const auto pid2 : PROPS_TO_SEED) {
                const auto [pname2, pval2] = get_prop_name_val(api, dprops, pid2);
                std::copy(pval2.begin(), pval2.end(), std::back_inserter(msgbuff));
            }
            std::copy(pval.begin(), pval.end(), std::back_inserter(msgbuff));
            msgbuff.push_back('\0');
            std::vector<char> buffer;
            anonymize(buffer, msgbuff);
            dprops[pname] = &buffer[0];
        }
    }
}

//  Anonymize the properties defined in the config file. If the values are already
//  anonymized, do nothing
void anonymize_all_props(const json& api, json& props)
{
    for (auto& [sdid, dprops] : props.items()) {
        anonymize_dev_props(api, dprops);
    }
}

//  print functions (miscellanous)
//------------------------------------------------------------------------------
//  Print OpenVR info.
void print_openvr(const json& jd, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();
    if (verb >= vdef) {
        iprint(sf, "OpenVR runtime: {:s}\n", jd[j_rt_path].get<std::string>());
        iprint(sf, "OpenVR version: {:s}\n", jd[j_rt_ver].get<std::string>());
    }
}

//  print functions (devices and properties)
//------------------------------------------------------------------------------
//  Print the property name to stdout.
inline void prop_head_out(vr::ETrackedDeviceProperty pid, const std::string& name,
                          bool is_array, int ind, int ts)
{
    iprint(ind * ts, "{:>4d} : {:s}{:s} = ", pid, name, is_array ? "[]" : "");
}

//  Print (non-error) value of an Array type property.
void print_array_type(const std::string& pname, const json& pval, int ind, int ts)
{
    const auto sf = ind * ts;
    // parse the name to get the type
    const auto [basename, ptype_name, ptype, is_array] = basevr::parse_prop_name(pname);

    switch (ptype) {
        case basevr::PropType::Float:
            print_tensor<double, 1>(pval.get<hvector_t>(), ind, ts);
            break;
        case basevr::PropType::Int32:
            print_tensor<int32_t, 1>(pval.get<xt::xtensor<int32_t, 1>>(), ind, ts);
            break;
        case basevr::PropType::Uint64:
            print_tensor<uint64_t, 1>(pval.get<xt::xtensor<uint64_t, 1>>(), ind, ts);
            break;
        case basevr::PropType::Bool:
            print_tensor<bool, 1>(pval.get<xt::xtensor<bool, 1>>(), ind, ts);
            break;
        case basevr::PropType::Matrix34:
            print_tensor<double, 3>(pval.get<xt::xtensor<double, 3>>(), ind, ts);
            break;
        case basevr::PropType::Matrix44:
            print_tensor<double, 3>(pval.get<xt::xtensor<double, 3>>(), ind, ts);
            break;
        case basevr::PropType::Vector2:
            print_tensor<double, 2>(pval.get<xt::xtensor<double, 2>>(), ind, ts);
            break;
        case basevr::PropType::Vector3:
            print_tensor<double, 2>(pval.get<xt::xtensor<double, 2>>(), ind, ts);
            break;
        case basevr::PropType::Vector4:
            print_tensor<double, 2>(pval.get<xt::xtensor<double, 2>>(), ind, ts);
            break;
        default:
            const auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype_name);
            iprint(sf, ERR_MSG_FMT_JSON, msg);
            break;
    }
}

//  Print enumerated devices.
void print_devs(const json& api, const json& devs, int ind, int ts)
{
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();
    const auto sf = ind * ts;
    const auto sf1 = (ind + 1) * ts;

    iprint(sf, "Device enumeration:\n");
    hdevlist_t res;
    for (const auto [dev_id, dev_class] : devs.get<hdevlist_t>()) {
        const auto cname = api[j_classes][std::to_string(dev_class)].get<std::string>();
        iprint(sf1, "Found dev: id={:d}, class={:d}, name={:s}\n", dev_id, dev_class,
               cname);
    }
}

//  Print device properties.
void print_dev_props(const json& api, const json& dprops, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto jverb = g_cfg[j_verbosity];
    const auto verb_props = g_cfg[j_openvr][j_verbosity][j_properties];
    const auto verr = jverb[j_error].get<int>();

    for (const auto& [pname, pval] : dprops.items()) {
        const auto name2id = api[j_properties][j_name2id];
        // if there is a property which is no longer supported by current openvr_api.json
        // ignore it
        if (name2id.find(pname) == name2id.end()) {
            continue;
        }
        // convert string to the correct type
        const auto pid = name2id[pname].get<vr::ETrackedDeviceProperty>();
        // decode property type
        const auto [basename, ptype_name, ptype, is_array]
            = basevr::parse_prop_name(pname);
        // property verbosity level (if defined) or max
        int pverb;
        // property having an error attached?
        const auto nerr = pval.count(ERROR_PREFIX);

        if (nerr) {
            // the value is an error code, so print it out only if the verbosity
            // is at 'error' level
            pverb = verr;
        }
        else {
            // determine the "active" verbosity level for the current property
            if (verb_props.find(pname) != verb_props.end()) {
                // explicitly defined property
                pverb = verb_props[pname].get<int>();
            }
            else {
                // otherwise set requested verbosity to vmax
                pverb = jverb[j_max].get<int>();
            }
        }
        if (verb < pverb) {
            // do not print props which require higher verbosity than the current one
            continue;
        }

        // print the prop name
        prop_head_out(pid, basename, is_array, ind, ts);
        if (nerr) {
            const auto msg = pval[ERROR_PREFIX].get<std::string>();
            fmt::print(ERR_MSG_FMT_JSON, msg);
        }
        else if (is_array) {
            fmt::print("\n");
            print_array_type(pname, pval, ind + 1, ts);
        }
        else {
            switch (ptype) {
                case basevr::PropType::Bool:
                    fmt::print("{}\n", pval.get<bool>());
                    break;
                case basevr::PropType::String:
                    fmt::print("\"{:s}\"\n", pval.get<std::string>());
                    break;
                case basevr::PropType::Uint64:
                    fmt::print("{:#x}\n", pval.get<uint64_t>());
                    break;
                case basevr::PropType::Int32:
                    fmt::print("{}\n", pval.get<int32_t>());
                    break;
                case basevr::PropType::Float:
                    fmt::print("{:.6g}\n", pval.get<double>());
                    break;
                case basevr::PropType::Matrix34: {
                    fmt::print("\n");
                    const harray2d_t mat34 = pval;
                    print_harray(mat34, ind + 1, ts);
                    break;
                }
                default:
                    const auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype_name);
                    fmt::print(ERR_MSG_FMT_JSON, msg);
            }
        }
    }
}

//  Print all properties for all devices.
void print_all_props(const json& api, const json& props, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto vdef = g_cfg[j_verbosity][j_default].get<int>();

    for (const auto& [sdid, dprops] : props.items()) {
        const auto dclass
            = dprops["Prop_DeviceClass_Int32"].get<vr::ETrackedDeviceClass>();
        const auto dcname = api[j_classes][std::to_string(dclass)].get<std::string>();
        if (verb >= vdef) {
            iprint(sf, "[{:s}:{:s}]\n", sdid, dcname);
        }
        print_dev_props(api, dprops, verb, ind + 1, ts);
    }
}

//  OpenVR Processor class
//------------------------------------------------------------------------------
// Initialize the processor
bool Processor::init()
{
    if (!m_apiPath.empty()) {
        json oapi = read_json(m_apiPath);
        m_jApi = parse_json_oapi(oapi);
    }
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
        anonymize_all_props(m_jApi, m_jData[j_properties]);
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

    print_openvr(m_jData, verb, ind, ts);
    if (verb >= vdef)
        fmt::print("\n");

    // print the devices and the properties
    auto tverb = (mode == pmode::props || mode == pmode::all) ? verb : vsil;
    if (tverb >= vdef) {
        if (m_jData.find(j_devices) != m_jData.end()) {
            print_devs(m_jApi, m_jData[j_devices], ind, ts);
            fmt::print("\n");
        }
    }
    if (m_jData.find(j_properties) != m_jData.end()) {
        print_all_props(m_jApi, m_jData[j_properties], tverb, ind, ts);
        if (tverb >= vdef)
            fmt::print("\n");
    }

    // print all the geometry
    tverb = (mode == pmode::geom || mode == pmode::all) ? verb : vsil;
    if (m_jData.find(j_geometry) != m_jData.end()) {
        print_geometry(m_jData[j_geometry], tverb, ind, ts);
        if (tverb >= vdef)
            fmt::print("\n");
    }
}

// Clean up the data before saving
void Processor::purge()
{
    if (m_jData.find(j_properties) != m_jData.end()) {
        purge_errors(m_jData[j_properties]);
    }
}

// Return OpenVR subystem ID
std::string Processor::get_id()
{
    return j_openvr;
}

// Return OpenVR data
json& Processor::get_data()
{
    return m_jData;
}

} // namespace openvr
