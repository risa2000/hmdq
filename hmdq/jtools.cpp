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

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <string>
#include <vector>

#include <fmt/format.h>

#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include <botan/blake2b.h>
#include <botan/filters.h>
#include <botan/hex.h>
#include <botan/pipe.h>

#include "config.h"
#include "jtools.h"

#include "fifo_map_fix.h"

//  globals
//------------------------------------------------------------------------------
//  properties to hash for PROPS_TO_HASH to "seed" (differentiate) same S/N from
//  different manufacturers (in this order)
const hproplist_t PROPS_TO_SEED
    = {vr::Prop_ManufacturerName_String, vr::Prop_ModelNumber_String};

//  OpenVR API loader
//------------------------------------------------------------------------------
//  Parse OpenVR JSON API definition, where jd = json.load("openvr_api.json")
json parse_json_oapi(const json& jd)
{
    json tdprops;
    json tdcls;
    for (const auto& e : jd["enums"]) {
        if (e["enumname"].get<std::string>() == "vr::ETrackedDeviceProperty") {
            for (const auto& v : e["values"]) {
                const auto name = v["name"].get<std::string>();
                // val type is actually vr::ETrackedDeviceProperty
                const auto val = std::stoi(v["value"].get<std::string>());
                const auto cat = static_cast<int>(val) / 1000;
                tdprops[std::to_string(cat)][std::to_string(val)] = name;
                tdprops["name2id"][name] = val;
            }
        }
        else if (e["enumname"].get<std::string>() == "vr::ETrackedDeviceClass") {
            for (const auto& v : e["values"]) {
                auto name = v["name"].get<std::string>();
                // val type is actually vr::ETrackedDeviceClass
                const auto val = std::stoi(v["value"].get<std::string>());
                const auto fs = name.find('_');
                if (fs != std::string::npos) {
                    name = name.substr(fs + 1, std::string::npos);
                }
                tdcls[std::to_string(val)] = name;
            }
        }
    }
    return json({{"classes", tdcls}, {"props", tdprops}});
}

//  functions
//------------------------------------------------------------------------------
//  Return property name and value for given property ID from device properties.
std::tuple<std::string, std::string>
get_prop_name_val(const json& api, const json& dprops, vr::ETrackedDeviceProperty pid)
{
    const auto cat = static_cast<int>(pid) / 1000;
    const auto pname
        = api["props"][std::to_string(cat)][std::to_string(pid)].get<std::string>();
    auto pval = std::string();
    // return empty string if the value does not exist or is not string (i.e. is an error)
    if (dprops.find(pname) != dprops.end() && dprops[pname].is_string())
        pval = dprops[pname].get<std::string>();
    return {pname, pval};
}

//  Anonymize properties for particular tracked device.
void anonymize_dev_props(const json& api, json& dprops)
{
    // get the list of properties to hash from the config file
    const hproplist_t props_to_hash = g_cfg["control"]["anon_props"].get<hproplist_t>();
    const std::string anon_prefix(ANON_PREFIX);
    for (const auto pid : props_to_hash) {
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

//  Calculate the hash over the JSON string dump using Blake2b(CHKSUM_BITSIZE).
//  The returned value is an upper case string of a binhex encoded hash.
std::string calculate_checksum(const json& jd)
{
    Botan::Pipe pipe(new Botan::Hash_Filter(fmt::format("Blake2b({:d})", CHKSUM_BITSIZE)),
                     new Botan::Hex_Encoder);
    // use the most efficient form for checksum (indent=-1)
    pipe.process_msg(jd.dump());
    return pipe.read_all_as_string();
}

//  Verify the checksum in the JSON dics (if there is one)
bool verify_checksum(const json& jd)
{
    if (!has_checksum(jd))
        return false;

    json jcopy = jd;
    const auto chksm = jcopy[CHECKSUM].get<std::string>();
    jcopy.erase(CHECKSUM);
    const auto vchksm = calculate_checksum(jcopy);
    return (chksm == vchksm);
}

//  Anonymize the message in `in` to `out`
void anonymize(std::vector<char>& out, const std::vector<char>& in)
{
    auto b2b = Botan::BLAKE2b(ANON_BITSIZE);
    const std::string prefix(ANON_PREFIX);
    b2b.update(reinterpret_cast<const uint8_t*>(&in[0]), std::strlen(&in[0]));
    // the size in chars is cipher size in bytes * 2 for BINHEX encoding
    // plus the anon prefix plus the terminating zero
    const auto anon_size = prefix.length() + ANON_BITSIZE / 8 * 2 + 1;
    if (out.size() < anon_size) {
        // resize out buffer to fit the hash
        out.resize(anon_size);
    }
    const auto hash_bh = Botan::hex_encode(b2b.final());
    std::copy(prefix.begin(), prefix.end(), out.begin());
    std::copy(hash_bh.begin(), hash_bh.end(), out.begin() + prefix.length());
    out[anon_size - 1] = '\0';
}
