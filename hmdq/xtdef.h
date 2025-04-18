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

#include <sstream>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <xtensor/containers/xarray.hpp>
#include <xtensor/containers/xtensor.hpp>
#include <xtensor/io/xio.hpp>

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
std::vector<std::string> format_tensor(const xt::xtensor<T, N>& a)
{
    // stringstream to dump the xarray
    std::stringstream temp;
    temp << a;

    std::vector<std::string> res;
    for (std::string line; getline(temp, line);) {
        res.push_back(line);
    }
    return res;
}

inline void print_multiline(const std::vector<std::string>& lines, int ind, int ts)
{
    const auto sf = ind * ts;
    for (const auto line : lines) {
        iprint(sf, "{}\n", line);
    }
}

//  Indent print xarray.
template<typename T, int N>
void print_tensor(const xt::xtensor<T, N>& a, int ind, int ts)
{
    const auto fval = format_tensor<T, N>(a);
    print_multiline(fval, ind, ts);
}

inline void print_harray(const harray2d_t& a, int ind, int ts)
{
    print_tensor<double, 2>(a, ind, ts);
}
