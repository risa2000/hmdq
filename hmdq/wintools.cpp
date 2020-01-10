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

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <cassert>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/os.h>

#include <windows.h>
#include <winver.h>

#include "wintools.h"

//  locals
//------------------------------------------------------------------------------
static UINT l_con_cp = ::GetConsoleOutputCP();
constexpr const char* SYS_ERROR_FMT = "System error [{:s}]";
constexpr size_t BUFF_SIZE = 256;

//  functions
//------------------------------------------------------------------------------
//  Create 'system error' exception
inline auto sys_error(const char* message)
{
    const DWORD error = ::GetLastError();
    // This one uses specific format, since the fmt::windows_error output includes also
    // the Windows error message (which is not obvious from the format alone).
    return fmt::windows_error(error, SYS_ERROR_FMT, message);
}

//  Print system error message for ::GetLastError to stderr, add `message` as contextual
//  info.
bool print_sys_error(const char* message)
{
    const auto werr = sys_error(message);
    fmt::print(stderr, werr.what());
    return true;
}

//  Return command line arguments as Windows Unicode strings (wstrings).
std::vector<std::wstring> get_wargs()
{
    int wargc = 0;
    wchar_t** wargv = ::CommandLineToArgvW(::GetCommandLineW(), &wargc);
    if (nullptr == wargv) {
        throw sys_error("CommandLineToArgvW failed");
    }
    std::vector<std::wstring> res(wargc);
    std::transform(wargv, wargv + wargc, res.begin(),
                   [](const auto& wa) { return std::wstring(wa); });
    ::LocalFree(wargv);
    return res;
}

//  Convert Windows Unicode zero termintad string to UTF-8 zero terminated string.
std::string wstr2utf8(const wchar_t* wstr)
{
    static std::vector<char> buffer(BUFF_SIZE);
    auto buffsize
        = ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &buffer[0], 0, nullptr, nullptr);
    if (buffsize > buffer.size()) {
        buffer.resize(buffsize);
    }
    buffsize = ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &buffer[0], buffer.size(),
                                     nullptr, nullptr);
    return std::string(&buffer[0]);
}

//  Convert wstring list of args to list of UTF-8 strings.
std::vector<std::string> wargs_to_u8args(const std::vector<std::wstring>& wargs)
{
    std::vector<std::string> res(wargs.size());
    std::transform(wargs.cbegin(), wargs.cend(), res.begin(),
                   [](const auto& w) { return wstr2utf8(w.c_str()); });
    return res;
}

//  Return command line arguments as UTF-8 string list (in vector).
std::vector<std::string> get_u8args()
{
    return wargs_to_u8args(get_wargs());
}

//  Get C-like args array from the list of strings (in a vector).
std::tuple<std::vector<size_t>, std::vector<char>>
get_c_argv(const std::vector<std::string>& args)
{
    size_t vsize = std::transform_reduce(args.cbegin(), args.cend(), size_t(0),
                                         std::plus<size_t>(),
                                         [](const auto& a) { return a.size() + 1; });
    std::vector<char> buffer(vsize);
    std::vector<size_t> ptrs;
    auto parg = &buffer[0];

    for (const auto& astr : args) {
        strcpy_s(parg, vsize - (parg - &buffer[0]), astr.c_str());
        ptrs.push_back(parg - &buffer[0]);
        parg += astr.size() + 1;
    }
    return {ptrs, buffer};
}

//  Enum code page callback, should set the CP specified in `l_con_cp`.
BOOL CALLBACK EnumCodePagesProcA(_In_ LPSTR lpCodePageString)
{
    if (std::atoi(lpCodePageString) == l_con_cp) {
        ::SetConsoleOutputCP(l_con_cp);
        return FALSE;
    }
    return TRUE;
}

//  Set console output code page, if installed.
void set_console_cp(unsigned int codepage)
{
    if (l_con_cp != codepage) {
        l_con_cp = codepage;
        ::EnumSystemCodePagesA(EnumCodePagesProcA, CP_INSTALLED);
    }
}

//  Get OS version or "n/a" if the attempt fails (print error in DEBUG build).
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
                assert(print_sys_error(rpath));
            }
        }
        else {
            assert(print_sys_error(std::to_string(size).c_str()));
        }
    }
    else {
        assert(print_sys_error(fos));
    }
    return "n/a";
}
