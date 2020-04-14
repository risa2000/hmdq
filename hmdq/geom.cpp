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

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <limits>

#include <xtensor/xarray.hpp>
#include <xtensor/xbuilder.hpp>
#include <xtensor/xview.hpp>

#include "geom.h"

//  functions
//------------------------------------------------------------------------------
//  Calculate matrix multiplication of two 2D arrays.
harray2d_t matmul(const harray2d_t& a1, const harray2d_t& a2)
{
    HMDQ_ASSERT(a1.shape(1) == a2.shape(0));
    const xt::xtensor<double, 2>::shape_type shape = {a1.shape(0), a2.shape(1)};
    harray2d_t res = xt::empty<double>(shape);
    for (size_t row = 0, erow = a1.shape(0); row < erow; ++row) {
        for (size_t col = 0, ecol = a2.shape(1); col < ecol; ++col) {
            res(row * ecol + col)
                = dot_prod(xt::view(a1, row), xt::view(a2, xt::all(), col));
        }
    }
    return res;
}

//  Calculate two points where two segments are closest:
//  seg1 = (a1, a2), seg2 = (b1, b2),
//  where a1, a2, b1, b2 are the respective endpoints of the segments.
//  Return "empty" vectors, if the segments are parallel.
hvecpair_t seg_seg_int(const hvector_t& a1, const hvector_t& a2, const hvector_t& b1,
                       const hvector_t& b2)
{
    // if some points directly compares do not bother with the rest
    if (a1 == b1) {
        return {a1, b1};
    }
    else if (a1 == b2) {
        return {a1, b2};
    }
    if (a2 == b1) {
        return {a2, b1};
    }
    else if (a2 == b2) {
        return {a2, b2};
    }

    // build LR mat from vectors
    const auto mat = xt::stack(xt::xtuple(a2 - a1, b1 - b2, b1 - a1), 1);
    const auto deto = det_mat(xt::view(mat, xt::all(), xt::keep(0, 1)));
    if (abs(deto) < EPS_100) {
        // return "empty" vectors if the determinant -> 0
        return {hvector_t(), hvector_t()};
    }

    const auto detu = det_mat(xt::view(mat, xt::all(), xt::keep(2, 1)));
    const auto detv = det_mat(xt::view(mat, xt::all(), xt::keep(0, 2)));
    auto u = detu / deto;
    auto v = detv / deto;

    if (u < 0)
        u = 0;
    else if (u > 1)
        u = 1;

    if (v < 0)
        v = 0;
    else if (v > 1)
        v = 1;

    hvector_t ap = a1 + (a2 - a1) * u;
    hvector_t bp = b1 + (b2 - b1) * v;

    return {ap, bp};
}

//  Calculate points of intersection of segement (a1, a2) and the mesh edges.
harray2d_t seg_mesh_int(const hvector_t& a1, const hvector_t& a2, const harray2d_t& verts,
                        const hfaces_t& faces)
{
    hveclist_t points;
    for (auto const& f : faces) {
        for (size_t i = 0; i < f.size(); ++i) {
            const auto v1 = xt::view(verts, f[i], xt::all());
            const auto v2 = xt::view(verts, f[(i + 1) % f.size()], xt::all());
            const hvecpair_t tpts = seg_seg_int(a1, a2, v1, v2);
            if (tpts.first.size() > 0) {
                // if size > 0 => not "empty" vector
                if (point_dist(tpts.first, tpts.second) < EPS_100) {
                    points.push_back(tpts.first);
                }
            }
        }
    }
    if (points.size() == 0) {
        // return "empty" array
        return harray2d_t();
    }
    return build_array(points);
}

//  Find closest vertex from `verts` to point `pt`.
hvector_t find_closest(const hvector_t& pt, const harray2d_t& verts)
{
    double dmin = std::numeric_limits<double>::max();
    hvector_t res;
    for (size_t i = 0; i < verts.shape(0); ++i) {
        const auto v = xt::view(verts, i);
        double dist = norm(v - pt);
        if (dist < dmin) {
            res = v;
            dmin = dist;
        }
    }
    return res;
}

//  Calculate the area of the triangle given by the vertices.
double area_triangle(const hvector_t& v1, const hvector_t& v2, const hvector_t& v3)
{
    const auto ab = v2 - v1;
    const auto ac = v3 - v1;
    return sqrt(dot_prod(ab, ab) * dot_prod(ac, ac) - pow(dot_prod(ab, ac), 2)) / 2;
}

//  Calculate the mesh area from given triangles. Each consequtive 3 vertices
//  define one triangle.
double area_mesh_raw(const harray2d_t& verts)
{
    double a = 0;
    for (size_t i = 0, e = verts.shape(0); i < e; i += 3) {
        a += area_triangle(xt::view(verts, i), xt::view(verts, i + 1),
                           xt::view(verts, i + 2));
    }
    return a;
}

//  Calculate the mesh area from given triangles. Triangle are specified by vertices
//  indexed by an index array.
double area_mesh_tris_idx(const harray2d_t& verts, const hfaces_t& tris)
{
    double a = 0;
    for (const auto& face : tris) {
        a += area_triangle(xt::view(verts, face[0]), xt::view(verts, face[1]),
                           xt::view(verts, face[2]));
    }
    return a;
}
