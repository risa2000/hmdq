/******************************************************************************
 * HMDQ Tools - tools for an OpenVR HMD and other hardware introspection      *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2019, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#pragma once

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <nlohmann/fifo_map.hpp>
#include <nlohmann/json.hpp>

namespace fix {

// A workaround which allows using fifo_map as a custom (ordered) map,
// we are just ignoring the 'less' compare
template<class K, class V, class dummy_compare, class A>
using my_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using fix_json = nlohmann::basic_json<my_fifo_map>;

} // namespace fix

using json = fix::fix_json;
