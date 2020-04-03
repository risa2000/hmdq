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

#include "base_classes.h"

#include "fifo_map_fix.h"

namespace oculus {

//  OculusVR Processor class
//------------------------------------------------------------------------------
class Processor : public BaseVRProcessor
{
  public:
    Processor(const std::shared_ptr<json>& pjdata) : m_pjData(pjdata) {}

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
    virtual std::shared_ptr<json> get_data() override;

  private:
    // Collected data
    std::shared_ptr<json> m_pjData;
};

} // namespace oculus
