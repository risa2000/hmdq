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
#include <iostream>
#include <vector>

#include <xtensor/xio.hpp>
#include <xtensor/xview.hpp>

#include "utils.h"
#include "xtdef.h"

//  debug functions
//------------------------------------------------------------------------------
//  Write the shape of the harray to the stdout.
void dump_shape(const char* name, const harray_t& ha)
{
    std::vector<size_t> vshape(ha.dimension());
    auto shape = ha.shape();
    std::copy(shape.cbegin(), shape.cend(), vshape.begin());
    std::cout << name << ".shape() = " << vshape << '\n';
}

//  Write the shape and the content of the harray to the stdout.
void dump_ha(const char* name, const harray_t& ha)
{
    dump_shape(name, ha);
    std::cout << name << " = " << ha << '\n';
}

//  functions
//------------------------------------------------------------------------------
//  Build the 2D-array from std::vector of 1D-arrays.
harray2d_t build_array(const hveclist_t& vecs)
{
    // at least one vector should always be present
    HMDQ_ASSERT(vecs.size() > 0);
    harray2d_t res({vecs.size(), vecs[0].shape(0)});
    for (size_t i = 0, e = vecs.size(); i < e; ++i) {
        xt::view(res, i) = vecs[i];
    }
    return res;
}

//  Indent print xarray.
void print_array(const harray_t& a, int ind, int ts)
{
    // string fill
    std::string sf(ts * ind, ' ');
    // stringstream to dump the xarray
    std::stringstream temp;
    temp << a;

    for (std::string line; getline(temp, line);) {
        std::cout << sf << line << '\n';
    }
}
