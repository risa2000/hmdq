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

#include <map>
#include <string>

#include "jkeys.h"

//  Print friendly names for keys
//------------------------------------------------------------------------------
const std::map<std::string, std::string> j_key2pretty
    = {{j_max_fov, "Maximal FOV"}, {j_default_fov, "Default FOV"}};

const std::string& get_jkey_pretty(const std::string& jkey)
{
    const auto iter = j_key2pretty.find(jkey);
    if (iter != j_key2pretty.cend()) {
        return iter->second;
    }
    else {
        return jkey;
    }
}
