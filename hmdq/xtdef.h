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

#pragma once

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <xtensor/xarray.hpp>
#include <xtensor/xtensor.hpp>

#include "utils.h"

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

//  debug functions
//------------------------------------------------------------------------------
//  Write the shape of the harray to the stdout.
void dump_shape(const char* name, const harray_t& ha);

//  Write the shape of the xtensor to the stdout.
template<typename T, int N>
void dump_shape(const char* name, const xt::xtensor<T, N>& ha)
{
    std::vector<size_t> vshape(ha.dimension());
    auto shape = ha.shape();
    std::copy(shape.cbegin(), shape.cend(), vshape.begin());
    std::cout << name << ".shape() = " << vshape << '\n';
}

//  Write the shape of the xarray to the stdout.
template<typename T>
void dump_shape(const char* name, const xt::xarray<T>& ha)
{
    std::vector<size_t> vshape(ha.dimension());
    auto shape = ha.shape();
    std::copy(shape.cbegin(), shape.cend(), vshape.begin());
    std::cout << name << ".shape() = " << vshape << '\n';
}

//  Write the shape and the content of the harray to the stdout:
void dump_ha(const char* name, const harray_t& ha);

//  Write the shape and the content of the xtensor to the stdout.
template<typename T, int N>
void dump_ha(const char* name, const xt::xtensor<T, N>& ha)
{
    dump_shape(name, ha);
    std::cout << name << " = " << ha << '\n';
}

//  Write the shape and the content of the xarray to the stdout.
template<typename T>
void dump_ha(const char* name, const xt::xarray<T>& ha)
{
    dump_shape(name, ha);
    std::cout << name << " = " << ha << '\n';
}

//  utility functions
//------------------------------------------------------------------------------
//  Build the 2D-array from std::vector of 1D-arrays.
harray2d_t build_array(const hveclist_t& vecs);

//  Indent print xarray.
void print_array(const harray_t& a, int ind, int ts);

template<typename T, int N>
void print_array(const xt::xtensor<T, N>& a, int ind, int ts)
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

//  Indent print T-typed xarray.
template<typename T>
void print_array(const xt::xarray<T>& a, int ind, int ts)
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
