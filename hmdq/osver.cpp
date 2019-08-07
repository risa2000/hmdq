/******************************************************************************
 * HMDQ - Query tool for an OpenVR HMD and some other hardware                *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2019, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <cassert>
#include <sstream>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <windows.h>
#include <winver.h>

#include "osver.h"

//  Print system error message for ::GetLastError to stderr, add `msg` as contextual info
bool error(const std::string& msg)
{
    const DWORD err = ::GetLastError();
    // this one uses specific format, since the fmt::windows_error output includes also
    // the Windows error message (which is not obvious from the format alone).
    const auto werr = fmt::windows_error(err, "Error: [{:s}]", msg);
    fmt::print(stderr, werr.what());
    return true;
}

//  Get OS version or "n/a" if the attempt fails (print error in DEBUG build)
std::string get_os_ver()
{
    LPCSTR fos = "ntoskrnl.exe";
    DWORD handle = 0;
    DWORD size = ::GetFileVersionInfoSizeA(fos, &handle);
    if (size) {
        std::vector<uint8_t> buffer(size);
        BOOL res = ::GetFileVersionInfoA(fos, 0, size, static_cast<LPVOID>(&buffer[0]));
        if (res) {
            LPVOID info = nullptr;
            UINT len = 0;
            const auto rpath = "\\";
            res = ::VerQueryValueA(static_cast<LPCVOID>(&buffer[0]), rpath, &info, &len);
            if (res) {
                const VS_FIXEDFILEINFO* fi = static_cast<const VS_FIXEDFILEINFO*>(info);
                return fmt::format("{:d}.{:d}.{:d}.{:d}", HIWORD(fi->dwProductVersionMS),
                                   LOWORD(fi->dwProductVersionMS),
                                   HIWORD(fi->dwProductVersionLS),
                                   LOWORD(fi->dwProductVersionLS));
            }
            else {
                assert(error(rpath));
            }
        }
        else {
            assert(error(std::to_string(size)));
        }
    }
    else {
        assert(error(fos));
    }
    return "n/a";
}
