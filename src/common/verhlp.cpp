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

#include <string>
#include <tuple>

//  locals
//------------------------------------------------------------------------------
static constexpr const char DOT = '.';

//  functions
//------------------------------------------------------------------------------
//  Get the first number from the version string.
std::tuple<int, size_t> first_num(const std::string& vs, size_t pos)
{
    if (pos == vs.size()) {
        return {0, pos};
    }
    const auto dpos = vs.find(DOT, pos);
    int num = 0;
    if (dpos == 0) {
        // starts with DOT, assume 0
        pos += 1;
    } else if (dpos == std::string::npos) {
        // did not find DOT, it is all the number
        num = std::stoi(vs.substr(pos));
        pos = vs.size();
    } else {
        // parse the number until dot
        num = std::stoi(vs.substr(pos, dpos - pos));
        pos = dpos + 1;
    }
    return {num, pos};
}

// Compare two versions `va` and `vb`.
// Return:
//  -1 : va < vb
//   0 : va == vb
//   1 : va > vb
int comp_ver(const std::string& va, const std::string& vb)
{
    size_t posa = 0;
    size_t posb = 0;
    int res = 0;

    while (posa < va.size() || posb < vb.size()) {
        const auto [numa, nposa] = first_num(va, posa);
        const auto [numb, nposb] = first_num(vb, posb);
        if (numa < numb) {
            res = -1;
            break;
        } else if (numa > numb) {
            res = 1;
            break;
        }
        posa = nposa;
        posb = nposb;
    }
    return res;
}
