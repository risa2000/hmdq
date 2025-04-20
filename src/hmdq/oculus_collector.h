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

#pragma once

#include <common/base_classes.h>
#include <common/jkeys.h>
#include <common/json_proxy.h>

#include <OVR_CAPI.h>

#include <memory>
#include <string>

namespace oculus {

//  OculusVR Collector class
//------------------------------------------------------------------------------
class Collector : public BaseVRCollector
{
  public:
    Collector(ovrInitFlags initFlags)
        : BaseVRCollector(j_oculus, std::make_shared<json>()), m_initFlags(initFlags),
          m_session(nullptr), m_inited(false)
    {}
    virtual ~Collector() override;

  public:
    // Check if the OculusVR subsystem is present and initialize it
    // Return: true if present and initialized, otherwise false
    virtual bool try_init() override;
    // Collect the OculusVR subsystem data
    virtual void collect() override;
    // Return the last OculusVR subsystem error
    virtual int get_last_error() const override;
    // Return the last OculusVR subsystem error message
    virtual std::string get_last_error_msg() const override;

  public:
    // Shutdown the OculusVR subsystem
    void shutdown();

  private:
    // Check the error and get the context info.
    bool check_failure(ovrResult ores);

  private:
    // Lib was initialized flag
    bool m_inited;
    // OculusVR init flags
    ovrInitFlags m_initFlags;
    // OculusVR graphics LUID
    ovrGraphicsLuid m_graphicsLuid;
    // OculusVR session
    ovrSession m_session;
    // The last OculusVR error
    ovrResult m_error;
    // Error info corresponding to the last error
    ovrErrorInfo m_errorInfo;
    // ovrHmdDesc
    std::unique_ptr<ovrHmdDesc> m_pHmdDesc;
};

} // namespace oculus
