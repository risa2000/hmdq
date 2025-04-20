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

#include <common/xtdef.h>
#include <common/except.h>

#include <xtensor/xview.hpp>

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
