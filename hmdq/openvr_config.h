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

#include "base_classes.h"

#include "fifo_map_fix.h"

namespace openvr {

//  OpenVR Config class (default config)
//------------------------------------------------------------------------------
class Config : public BaseVRConfig
{
  public:
    Config();

  public:
    // Return VR subystem ID
    virtual std::string get_id() override;
    // Return VR subystem default config
    virtual json& get_data() override;

  private:
    // Config data
    json m_jConfig;
};

} // namespace openvr
