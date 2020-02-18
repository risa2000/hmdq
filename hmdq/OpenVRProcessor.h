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

#define BUILD_OPENVR_STATIC
#include <openvr/openvr.h>

#include "BaseVR.h"

#include "fifo_map_fix.h"

//  OpenVRProcessor class
//------------------------------------------------------------------------------
//  Printer for OpenVR subsystem
class OpenVRProcessor : public BaseVRProcessor
{
  public:
    OpenVRProcessor(const std::filesystem::path& apiPath, json& jdata)
        : m_jData(jdata), m_apiPath(apiPath)
    {}
    OpenVRProcessor(const json& japi, json& jdata) : m_jData(jdata), m_jApi(japi) {}

  public:
    // Initialize the processor
    virtual bool init() override;
    // Calculate complementary data
    virtual void calculate() override;
    // Anonymize sensitive data
    virtual void anonymize() override;
    // Print the collected data
    // mode: props, geom, all
    // verb: verbosity
    // ind: indentation
    // ts: indent (tab) size
    virtual void print(pmode mode, int verb, int ind, int ts) override;
    // Clean up the data before saving
    virtual void purge() override;
    // Return OpenVR subystem ID
    virtual std::string get_id() override;
    // Return OpenVR subystem data
    virtual json& get_data() override;

  private:
    // API extract
    json m_jApi;
    // Collected data
    json& m_jData;
    // OpenVR API JSON file path
    std::filesystem::path m_apiPath;
};
