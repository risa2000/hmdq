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

#include "xtdef.h"

#include "fifo_map_fix.h"

//  functions
//------------------------------------------------------------------------------
//  Return transform matrix from UV (mesh space) to LRTB (lrtb projection plane).
inline harray2d_t uv2lrbt(double l, double r, double b, double t)
{
    return harray2d_t({{r - l, 0, l}, {0, t - b, b}});
}

//  Transform vertices from UV space into LRBT space.
harray2d_t verts_uv2lrbt(const harray2d_t& verts, double l, double r, double b, double t);

//  Build 2D points/vectors (depends on `bpt`) for LRBT rectangle.
harray2d_t build_lrbt_quad_2d(const json& raw, double norm = 1.0);

//  Calculate optimized HAM mesh topology
json calc_opt_ham_mesh(const json& ham_mesh);

//  Calculate partial FOVs for the projection.
json calc_fov(const json& raw, const json& mesh, const harray2d_t* rot = nullptr);

//  Calculate total FOV, vertical, horizontal and diagonal.
json calc_total_fov(const json& fov_head);

//  Calculate the angle of the canted views and the IPD from eye to head transformation
//  matrices.
json calc_view_geom(const json& e2h);

//  Calculate the additional data in the geometry data object (json)
json calc_geometry(const json& jd);
