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
#include <cmath>

#include "xtdef.h"

//  globals
//------------------------------------------------------------------------------
constexpr double EPS = std::numeric_limits<double>::epsilon() * 100;

//  functions
//------------------------------------------------------------------------------
//  Compute degrees out of radians.
inline double degrees(double rad)
{
    return (rad * 180) / xt::numeric_constants<double>::PI;
}

//  Compute dot product of two 2-D vectors.
inline double dot_prod(const harray_t& v1, const harray_t& v2)
{
    auto sres = xt::sum(v1 * v2);
    return sres[0];
}

//  Compute determinant of 2x2 matrix.
inline double det_mat_2x2(const harray2d_t& m)
{
    // make sure these are indeed 2x2 matrices
    HMDQ_ASSERT(m.dimension() == 2 && m.shape(0) == 2 && m.shape(1) == 2);
    return m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1);
}

//  Compute determinant of the matrix.
inline double det_mat(const harray2d_t& m)
{
    // only 2x2 matrices are supported by now
    HMDQ_ASSERT(m.dimension() == 2 && m.shape(0) == 2 && m.shape(1) == 2);
    return det_mat_2x2(m);
}

//  Compute vector length.
inline double norm(const harray_t& v) { return sqrt(dot_prod(v, v)); }

//  Compute the angle between the two vectors in radians.
inline double angle(const hvector_t& v1, const hvector_t& v2)
{
    double tres = dot_prod(v1, v2) / (norm(v1) * norm(v2));
    if (tres > 1.0) {
        tres = 1.0;
    }
    else if (tres < -1.0) {
        tres = -1.0;
    }
    return acos(tres);
}

//  Compute the angle between the two vectors in degrees.
inline double angle_deg(const hvector_t& v1, const hvector_t& v2)
{
    return degrees(angle(v1, v2));
}

//  Calculate the distance between two 3D points.
inline double point_dist(const hvector_t& p1, const hvector_t& p2)
{
    return norm(xt::eval(p1 - p2));
}

//  Calculate matrix multiplication of two 2D arrays.
harray2d_t matmul(const harray2d_t& a1, const harray2d_t& a2);

//  Calculate two points where two segments are closest:
//  seg1 = (a1, a2), seg2 = (b1, b2),
//  where a1, a2, b1, b2 are the respective endpoints of the segments.
//  Return "empty" vectors, if the segments are parallel.
hvecpair_t seg_seg_int(const hvector_t& a1, const hvector_t& a2, const hvector_t& b1,
                       const hvector_t& b2);

//  Calculate points of intersection of segement (a1, a2) and the mesh edges.
harray2d_t seg_mesh_int(const hvector_t& a1, const hvector_t& a2, const harray2d_t& verts,
                        const hfaces_t& faces);

//  Find closest vertex from `verts` to point `pt`.
hvector_t find_closest(const hvector_t& pt, const harray2d_t& verts);

//  Calculate the area of the triangle given by the vertices.
double area_triangle(const hvector_t& v1, const hvector_t& v2, const hvector_t& v3);

//  Calculate the mesh area from given triangles. Each consequtive 3 vertices
//  define one triangle.
double area_mesh(const harray2d_t& verts);
