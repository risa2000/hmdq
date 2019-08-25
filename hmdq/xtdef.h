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
#include <sstream>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <xtensor/xarray.hpp>
#include <xtensor/xio.hpp>
#include <xtensor/xtensor.hpp>

#include "fmthlp.h"

//  typedefs
//------------------------------------------------------------------------------
typedef xt::xtensor<double, 2> harray2d_t;
typedef xt::xarray<double> harray_t;
typedef xt::xtensor<double, 1> hvector_t;
typedef std::vector<size_t> hface_t;
typedef std::vector<hface_t> hfaces_t;
typedef std::vector<hvector_t> hveclist_t;
typedef std::pair<hvector_t, hvector_t> hvecpair_t;
typedef std::pair<size_t, size_t> hedge_t;
typedef std::vector<hedge_t> hedgelist_t;

//  utility functions
//------------------------------------------------------------------------------
//  Build the 2D-array from std::vector of 1D-arrays.
harray2d_t build_array(const hveclist_t& vecs);

//  Indent print xarray.
template<typename T, int N>
void print_tensor(const xt::xtensor<T, N>& a, int ind, int ts)
{
    const auto sf = ind * ts;
    // stringstream to dump the xarray
    std::stringstream temp;
    temp << a;

    for (std::string line; getline(temp, line);) {
        iprint(sf, "{}\n", line);
    }
}

inline void print_harray(const harray2d_t& a, int ind, int ts)
{
    print_tensor<double, 2>(a, ind, ts);
}
