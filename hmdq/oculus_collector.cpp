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

#include <OVR_CAPI.h>

#include "jkeys.h"
#include "oculus_collector.h"

#include "fifo_map_fix.h"

namespace oculus {

//  OculusVR Collector class
//------------------------------------------------------------------------------
Collector::~Collector()
{
    shutdown();
}

// Shutdown the OculusVR subsystem
void Collector::shutdown()
{
    if (nullptr != m_session) {
        ovr_Destroy(m_session);
        m_session = nullptr;
    }
    if (m_inited) {
        ovr_Shutdown();
        m_inited = false;
    }
}

// Check the error and get the context info.
bool Collector::check_failure(ovrResult ores)
{
    bool res = false;
    if (OVR_FAILURE(ores)) {
        ovr_GetLastErrorInfo(&m_errorInfo);
        res = true;
    }
    return res;
}

// Check if the OculusVR subsystem is present and initialize it
// Return: true if present and initialized, otherwise false
bool Collector::try_init()
{
    // merge 'custom' flags into init params
    ovrInitParams initParams
        = {ovrInit_RequestVersion | m_initFlags, OVR_MINOR_VERSION, NULL, 0, 0};
    m_error = ovr_Initialize(&initParams);
    if (check_failure(m_error)) {
        return false;
    }
    m_inited = true;

    m_error = ovr_Create(&m_session, &m_graphicsLuid);
    if (check_failure(m_error)) {
        shutdown();
        return false;
    }
    return true;
}

// Collect the OculusVR subsystem data
void Collector::collect()
{
    m_jData[j_rt_ver] = ovr_GetVersionString();
}

// Return OculusVR subystem ID
std::string Collector::get_id()
{
    return j_oculus;
}

// Return the last OculusVR subsystem error
int Collector::get_last_error()
{
    return static_cast<int>(m_error);
}

// Return the last OculusVR subsystem error message
std::string Collector::get_last_error_msg()
{
    return std::string(m_errorInfo.ErrorString);
}

// Return OculusVR data
json& Collector::get_data()
{
    return m_jData;
}

} // namespace oculus
