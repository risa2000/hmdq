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

#pragma once

#include <filesystem>
#include <string>

#include <openvr/openvr.h>

#include "base_classes.h"

#include "fifo_map_fix.h"

namespace openvr {

//  OpenVR Collector class
//------------------------------------------------------------------------------
class Collector : public BaseVRCollector
{
  public:
    Collector(const std::filesystem::path& apiPath, vr::EVRApplicationType appType)
        : BaseVRCollector(j_openvr, std::make_shared<json>()), m_appType(appType),
          m_ivrSystem(nullptr), m_err(vr::VRInitError_None), m_apiPath(apiPath),
          m_pjApi(std::make_shared<json>())
    {}
    virtual ~Collector() override;

  public:
    // Check if the OpenVR subsystem is present and initialize it
    // Return: true if present and initialized, otherwise false
    virtual bool try_init() override;
    // Collect the OpenVR subsystem data
    virtual void collect() override;
    // Return the last OpenVR subsystem error
    virtual int get_last_error() const override;
    // Return the last OpenVR subsystem error message
    virtual std::string get_last_error_msg() const override;

  public:
    // Return OpenVR API extract (for printer)
    virtual std::shared_ptr<json> get_xapi();
    // Shutdown the OpenVR subsystem
    void shutdown();

  private:
    // OpenVR app type to initialize the subsystem
    vr::EVRApplicationType m_appType;
    // Initialized system
    vr::IVRSystem* m_ivrSystem;
    // The last OpenVR error
    vr::EVRInitError m_err;
    // OpenVR API JSON file path
    std::filesystem::path m_apiPath;
    // API extract
    std::shared_ptr<json> m_pjApi;
};

} // namespace openvr
