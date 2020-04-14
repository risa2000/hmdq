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

#include <cassert>
#include <filesystem>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/os.h>

#include <windows.h>
#include <winver.h>

#include "compat.h"
#include "wintools.h"

//  locals
//------------------------------------------------------------------------------
static UINT l_con_cp;
constexpr const char* SYS_ERROR_FMT = "System error [{:s}]";
constexpr size_t BUFF_SIZE = 256;
constexpr size_t MAX_BUFF_SIZE = 2048;

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

//  Return the full path of the module (prog image if NULL)
std::filesystem::path get_module_path(HMODULE hMod)
{
    static std::vector<wchar_t> buffer(BUFF_SIZE);
    while (true) {
        auto nSize = ::GetModuleFileNameW(hMod, &buffer[0], buffer.size());
        if (0 == nSize) {
            throw sys_error(fmt::format("::GetModuleFileNameW failed on hModule = {}",
                                        reinterpret_cast<void*>(hMod))
                                .c_str());
        }
        else if (nSize == buffer.size()) {
            if (buffer.size() * 2 <= MAX_BUFF_SIZE) {
                buffer.resize(buffer.size() * 2);
                continue;
            }
            else {
                throw sys_error(
                    fmt::format("::GetModuleFileNameW path too long on hModule = {}",
                                reinterpret_cast<void*>(hMod))
                        .c_str());
            }
        }
        else {
            break;
        }
    }
    return std::filesystem::path(&buffer[0]);
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
std::tuple<std::shared_ptr<std::vector<char*>>, std::shared_ptr<std::vector<char>>>
get_c_argv(const std::vector<std::string>& args)
{
    auto ptrs = std::make_shared<std::vector<char*>>(args.size());
    auto buff = std::make_shared<std::vector<char>>();
    std::vector<size_t> offsets;

    for (const auto& astr : args) {
        offsets.push_back(buff->size());
        std::copy(astr.begin(), astr.end(), std::back_inserter<std::vector<char>>(*buff));
        buff->push_back(u8'\0');
    }
    //  now the buffer is stable, we can calculate the absolute addresses of arguments
    std::transform(offsets.begin(), offsets.end(), ptrs->begin(),
                   [&buff](const auto& offset) { return &(*buff)[0] + offset; });
    return {ptrs, buff};
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

//  Init console output code page
void init_console_cp()
{
    l_con_cp = ::GetConsoleOutputCP();
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

//  Return the full path of the executable which created the process
std::filesystem::path get_full_prog_path()
{
    return get_module_path(NULL);
}

//  Print command line arguments (for debugging purposes)
void print_u8args(std::vector<std::string> u8args)
{
    fmt::print("Command line arguments:\n");
    for (int i = 0, e = u8args.size(); i < e; ++i) {
        fmt::print("{:d}: {}\n", i, u8args[i]);
    }
    fmt::print("\n");
}
