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

#include "json_proxy.h"
#include "xtdef.h"

//  functions
//------------------------------------------------------------------------------
//  load or build verts and faces from recorded data
std::tuple<harray2d_t, hfaces_t, bool> calc_resolve_verts_and_faces(const json& ham_mesh);

//  Calculate optimized HAM mesh topology
json calc_opt_ham_mesh(const json& ham_mesh);

//  Calculate HAM area
double calc_ham_area(const json& ham_mesh);

//  Calculate partial FOVs for the projection (new version).
json calc_fov(const json& raw, const json& mesh, const harray2d_t* rot = nullptr);

//  Calculate total FOV, vertical, horizontal and diagonal.
json calc_total_fov(const json& fov_head);

//  Calculate the angle of the canted views and the IPD from eye to head transformation
//  matrices.
json calc_view_geom(const json& e2h);

//  Calculate the additional data in the geometry data object (json)
json calc_geometry(const json& jd);

//  Do sanity check on geometry data (Quest 2 - firmware major 10579)
//  Augment the JSON data with the error code if one is found.
bool geometry_sanity_check(json& geom);
