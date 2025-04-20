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

#include <filesystem>
#include <string>

namespace openvr {

//  OpenVR Processor class
//------------------------------------------------------------------------------
//  Printer for OpenVR subsystem
class Processor : public BaseVRProcessor
{
  public:
    Processor(const std::filesystem::path& apiPath, const std::shared_ptr<json>& pjdata)
        : BaseVRProcessor(j_openvr, pjdata), m_apiPath(apiPath)
    {}
    Processor(const std::shared_ptr<json>& pjapi, const std::shared_ptr<json>& pjdata)
        : BaseVRProcessor(j_openvr, pjdata), m_pjApi(pjapi)
    {}

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
    virtual void print(const print_options& opts, int ind, int ts) const override;
    // Clean up the data before saving
    virtual void purge() override;

  private:
    // OpenVR API JSON file path
    std::filesystem::path m_apiPath;
    // API extract
    std::shared_ptr<json> m_pjApi;
};

} // namespace openvr
