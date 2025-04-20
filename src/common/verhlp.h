/******************************************************************************
 * HMDQ Tools - tools for VR headsets and other hardware introspection        *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2019, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#pragma once

#include <string>

//  functions
//------------------------------------------------------------------------------
// Compare two versions `va` and `vb`.
// Return:
//  -1 : va < vb
//   0 : va == vb
//   1 : va > vb
int comp_ver(const std::string& va, const std::string& vb);
