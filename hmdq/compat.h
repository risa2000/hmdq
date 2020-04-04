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

#include <string>

//  functions
//------------------------------------------------------------------------------
//  Make std::string out of std::u8string for the compatibilty purposes
#if (__cplusplus <= 201703L)
inline const std::string& u8str2str(const std::string& u8str)
{
    return u8str;
}
#else
inline std::string u8str2str(const std::u8string& u8str)
{
    return std::string(reinterpret_cast<const char*>(&u8str[0]));
}
#endif
