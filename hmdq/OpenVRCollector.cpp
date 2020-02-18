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

#include "OpenVRCollector.h"
#include "jtools.h"
#include "ovrhlp.h"

//  OpenVRCollector class
//------------------------------------------------------------------------------
OpenVRCollector::~OpenVRCollector()
{
    shutdown();
}

// Return API extract (for printer)
const json& OpenVRCollector::get_xapi()
{
    return m_jApi;
}

// Shutdown the OpenVR subsystem
void OpenVRCollector::shutdown()
{
    if (nullptr != m_ivrSystem) {
        vr::VR_Shutdown();
        m_ivrSystem = nullptr;
    }
}

// Check if the OpenVR subsystem is present and initialize it
// Return: true if present and initialized, otherwise false
bool OpenVRCollector::try_init()
{
    if (!vr::VR_IsRuntimeInstalled()) {
        m_err = vr::VRInitError_Init_InstallationNotFound;
        return false;
    }
    bool res = false;
    auto [vrsys, error] = init_vrsys(m_appType);
    if (nullptr != vrsys) {
        m_ivrSystem = vrsys;
        res = true;
        json oapi = read_json(m_apiPath);
        m_jApi = parse_json_oapi(oapi);
    }
    m_err = error;
    return res;
}

// Collect the OpenVR subsystem data
void OpenVRCollector::collect()
{
    m_jData = get_openvr(m_ivrSystem, m_jApi);
}

// Return OpenVR subystem ID
std::string OpenVRCollector::get_id()
{
    return "openvr";
}

// Return the last OpenVR subsystem error
int OpenVRCollector::get_last_error()
{
    return static_cast<int>(m_err);
}

// Return the last OpenVR subsystem error message
std::string OpenVRCollector::get_last_error_msg()
{
    return std::string(vr::VR_GetVRInitErrorAsEnglishDescription(m_err));
}

// Return OpenVR data
json& OpenVRCollector::get_data()
{
    return m_jData;
}
