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

#include <algorithm>

#include <fmt/format.h>

#include <OVR_CAPI.h>

#include "jkeys.h"
#include "oculus_collector.h"
#include "oculus_common.h"
#include "oculus_props.h"

#include "fifo_map_fix.h"

namespace oculus {

//  constants
//------------------------------------------------------------------------------
constexpr const char* TRACKER_FMT = "tracker{}";

//  functions
//------------------------------------------------------------------------------
json get_devices(ovrSession session)
{
    json devs;
    const ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
    devs[j_hmd] = ((ovrHmd_None != hmdDesc.Type) ? 1 : 0);
    devs[j_trackers] = ovr_GetTrackerCount(session);
    devs[j_ctrl_types] = ovr_GetConnectedControllerTypes(session);
    return devs;
}

json get_hmd_props(ovrSession session)
{
    json res;
    const ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
    if (ovrHmd_None != hmdDesc.Type) {
        res[Prop::HmdType_Uint32] = hmdDesc.Type;
        res[Prop::ProductName_String] = hmdDesc.ProductName;
        res[Prop::Manufacturer_String] = hmdDesc.Manufacturer;
        res[Prop::VendorId_Uint16] = static_cast<uint16_t>(hmdDesc.VendorId);
        res[Prop::ProductId_Uint16] = static_cast<uint16_t>(hmdDesc.ProductId);
        res[Prop::SerialNumber_String] = hmdDesc.SerialNumber;
        res[Prop::FirmwareMajor_Uint16] = static_cast<uint16_t>(hmdDesc.FirmwareMajor);
        res[Prop::FirmwareMinor_Uint16] = static_cast<uint16_t>(hmdDesc.FirmwareMinor);
        res[Prop::AvailableHmdCaps_Uint32] = hmdDesc.AvailableHmdCaps;
        res[Prop::DefaultHmdCaps_Uint32] = hmdDesc.DefaultHmdCaps;
        res[Prop::AvailableTrackingCaps_Uint32] = hmdDesc.AvailableTrackingCaps;
        res[Prop::DefaultTrackingCaps_Uint32] = hmdDesc.DefaultTrackingCaps;
        res[Prop::DisplayRefreshRate_Float] = hmdDesc.DisplayRefreshRate;
    }
    return res;
}

json get_tracker_props(ovrSession session, unsigned int tnum)
{
    json res;
    ovrTrackerDesc trDesc = ovr_GetTrackerDesc(session, tnum);
    if (trDesc.FrustumHFovInRadians) {
        // sanity check for valid info
        res[Prop::FrustumHFovInRadians_Float] = trDesc.FrustumHFovInRadians;
        res[Prop::FrustumVFovInRadians_Float] = trDesc.FrustumVFovInRadians;
        res[Prop::FrustumFarZInMeters_Float] = trDesc.FrustumFarZInMeters;
        res[Prop::FrustumNearZInMeters_Float] = trDesc.FrustumNearZInMeters;
    }
    return {};
}

json get_controller_props(ovrSession session, ovrControllerType ctype)
{
    return {};
}

json get_properties(ovrSession session)
{
    json res;
    res[j_hmd] = get_hmd_props(session);
    const auto tcount = ovr_GetTrackerCount(session);
    if (tcount > 0) {
        for (unsigned int i = 0; i < tcount; ++i) {
            res[fmt::format(TRACKER_FMT, i).c_str()] = get_tracker_props(session, i);
        }
    }
    const auto ctrls = ovr_GetConnectedControllerTypes(session);
    for (const auto [mask, name] : g_bmControllerTypes) {
        if (0 != (ctrls & mask)) {
            res[name]
                = get_controller_props(session, static_cast<ovrControllerType>(mask));
        }
    }
    return res;
}

json get_geometry(ovrSession session)
{
    return {};
}

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
        = {static_cast<uint32_t>(ovrInit_RequestVersion) | m_initFlags, OVR_MINOR_VERSION,
           NULL, 0, 0};
    m_error = ovr_Initialize(&initParams);
    if (check_failure(m_error)) {
        (*m_pjData)[ERROR_PREFIX] = get_last_error_msg();
        return false;
    }
    m_inited = true;

    m_error = ovr_Create(&m_session, &m_graphicsLuid);
    if (check_failure(m_error)) {
        (*m_pjData)[ERROR_PREFIX] = get_last_error_msg();
        shutdown();
        return false;
    }
    return true;
}

// Collect the OculusVR subsystem data
void Collector::collect()
{
    (*m_pjData)[j_rt_ver] = ovr_GetVersionString();
    (*m_pjData)[j_devices] = get_devices(m_session);
    (*m_pjData)[j_properties] = get_properties(m_session);
    // (*m_pjData)[j_geometry] = get_geometry(m_session);
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
std::shared_ptr<json> Collector::get_data()
{
    return m_pjData;
}

} // namespace oculus
