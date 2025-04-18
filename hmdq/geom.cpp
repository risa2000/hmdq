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

#include <xtensor/views/xview.hpp>

#include "except.h"
#include "geom.h"

//  functions
//------------------------------------------------------------------------------
//  Calculate the area of the triangle given by the vertices.
double area_triangle(const hvector_t& v1, const hvector_t& v2, const hvector_t& v3)
{
    const auto ab = v2 - v1;
    const auto ac = v3 - v1;
    return sqrt(dot_prod(ab, ab) * dot_prod(ac, ac) - pow(dot_prod(ab, ac), 2)) / 2;
}

//  Calculate the mesh area from given triangles. Triangle are specified by vertices
//  indexed by an index array.
double area_mesh_tris_idx(const harray2d_t& verts, const hfaces_t& tris)
{
    double a = 0;
    for (const auto& face : tris) {
        HMDQ_ASSERT(face.size() == 3);
        a += area_triangle(xt::view(verts, face[0]), xt::view(verts, face[1]),
                           xt::view(verts, face[2]));
    }
    return a;
}
