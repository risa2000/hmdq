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

#include <common/xtdef.h>

#include <cmath>

//  functions
//------------------------------------------------------------------------------
//  Compute degrees out of radians.
inline double degrees(double rad)
{
    return (rad * 180) / xt::numeric_constants<double>::PI;
}

//  Compute dot product of two 2-D vectors.
template<typename TArray>
double dot_prod(const TArray& v1, const TArray& v2)
{
    auto sres = xt::sum(v1 * v2);
    return sres[0];
}

//  Compute vector length.
template<typename TArray>
double gnorm(const TArray& v)
{
    return sqrt(dot_prod(v, v));
}

//  Compute the angle between the two vectors in radians.
inline double angle(const hvector_t& v1, const hvector_t& v2)
{
    double tres = dot_prod(v1, v2) / (gnorm(v1) * gnorm(v2));
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
    return gnorm(xt::eval(p1 - p2));
}

//  Calculate the area of the triangle given by the vertices.
double area_triangle(const hvector_t& v1, const hvector_t& v2, const hvector_t& v3);

//  Calculate the mesh area from given triangles. Triangle are specified by vertices
//  indexed by an index array.
double area_mesh_tris_idx(const harray2d_t& verts, const hfaces_t& tris);

//  Calculate the mesh area from given triangles. Triangle are specified by vertices
//  indexed by an index array (using GEOS library)
double area_mesh_tris_idx_geos(const harray2d_t& verts, const hfaces_t& tris);
