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

#include "OpenVRProcessor.h"
#include "config.h"
#include "hmdview.h"
#include "jtools.h"
#include "ovrhlp.h"
#include "prtdata.h"

//  OpenVRProcessor class
//------------------------------------------------------------------------------
// Initialize the processor
bool OpenVRProcessor::init()
{
    if (!m_apiPath.empty()) {
        json oapi = read_json(m_apiPath);
        m_jApi = parse_json_oapi(oapi);
    }
    return true;
}

// Calculate the complementary data
void OpenVRProcessor::calculate()
{
    if (m_jData.find("geometry") != m_jData.end()) {
        m_jData["geometry"] = calc_geometry(m_jData["geometry"]);
    }
}

// Anonymize sensitive data
void OpenVRProcessor::anonymize()
{
    if (m_jData.find("properties") != m_jData.end()) {
        anonymize_all_props(m_jApi, m_jData["properties"]);
    }
}

// Print the collected data
// mode: props, geom, all
// verb: verbosity
// ind: indentation
// ts: indent (tab) size
void OpenVRProcessor::print(pmode mode, int verb, int ind, int ts)
{
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    const auto vsil = g_cfg["verbosity"]["silent"].get<int>();

    print_openvr(m_jData, verb, ind, ts);
    if (verb >= vdef)
        fmt::print("\n");

    // print the devices and the properties
    auto tverb = (mode == pmode::props || mode == pmode::all) ? verb : vsil;
    if (tverb >= vdef) {
        if (m_jData.find("devices") != m_jData.end()) {
            print_devs(m_jApi, m_jData["devices"], ind, ts);
            fmt::print("\n");
        }
    }
    if (m_jData.find("properties") != m_jData.end()) {
        print_all_props(m_jApi, m_jData["properties"], tverb, ind, ts);
        if (tverb >= vdef)
            fmt::print("\n");
    }

    // print all the geometry
    tverb = (mode == pmode::geom || mode == pmode::all) ? verb : vsil;
    if (m_jData.find("geometry") != m_jData.end()) {
        print_geometry(m_jData["geometry"], tverb, ind, ts);
        if (tverb >= vdef)
            fmt::print("\n");
    }
}

// Clean up the data before saving
void OpenVRProcessor::purge()
{
    if (m_jData.find("properties") != m_jData.end()) {
        purge_errors(m_jData["properties"]);
    }
}

// Return OpenVR subystem ID
std::string OpenVRProcessor::get_id()
{
    return "openvr";
}

// Return OpenVR data
json& OpenVRProcessor::get_data()
{
    return m_jData;
}
