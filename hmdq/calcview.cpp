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

#include <xtensor/xarray.hpp>
#include <xtensor/xbuilder.hpp>
#include <xtensor/xjson.hpp>
#include <xtensor/xview.hpp>

#include "calcview.h"
#include "except.h"
#include "geom.h"
#include "geom2.h"
#include "jkeys.h"
#include "json_proxy.h"
#include "optmesh.h"
#include "xtdef.h"

//  functions
//------------------------------------------------------------------------------
//  Calculate optimized HAM mesh topology
json calc_opt_ham_mesh(const json& ham_mesh)
{
    // raw vertices = three consecutive vertices define one triangle
    harray2d_t verts_raw;
    // faces (corresponding to verts_raw) either built or collected
    hfaces_t faces_raw;
    // faces raw computed or recorded?
    bool faces_raw_computed = false;

    // if the raw values are not explicitly present, the raw values are in "opt" values
    const char* j_verts = ham_mesh.contains(j_verts_raw) ? j_verts_raw : j_verts_opt;
    // if there are verts_opt then "enforce" faces_raw to be raw as potential faces_opt
    // should address verts_opt
    const char* j_faces = ham_mesh.contains(j_faces_raw) || ham_mesh.contains(j_verts_opt)
        ? j_faces_raw
        : j_faces_opt;

    verts_raw = ham_mesh[j_verts];

    // if there are already indexed vertices (Oculus) skip their creation
    if (!ham_mesh.contains(j_faces)) {
        // number of vertices must be divisible by 3 as each 3 defined one triangle
        HMDQ_ASSERT(verts_raw.shape(0) % 3 == 0);
        // build the trivial faces_raw for the triangles
        for (size_t i = 0, e = verts_raw.shape(0); i < e; i += 3) {
            faces_raw.push_back(hface_t({i, i + 1, i + 2}));
        }
        faces_raw_computed = true;
    }
    else {
        faces_raw = ham_mesh[j_faces].get<hfaces_t>();
    }
    // reduce duplicated vertices
    const auto& [verts_opt, n_faces] = reduce_verts(verts_raw, faces_raw);
    // do final faces optimization
    const auto faces_opt = reduce_faces(n_faces);

    // build the resulting JSON
    json res;

    // assume here that n_faces are 'faces_raw' and in fact triangles
    res[j_ham_area] = area_mesh_tris_idx(verts_opt, n_faces);

    if (verts_raw != verts_opt) {
        // save 'verts_raw' only if they differ from the optimized version
        res[j_verts_raw] = verts_raw;
    }
    if (faces_raw != faces_opt && !faces_raw_computed) {
        // save 'faces_raw' only if they differ from the optimized version
        res[j_faces_raw] = faces_raw;
    }
    res[j_verts_opt] = verts_opt;
    res[j_faces_opt] = faces_opt;
    return res;
}

//  Calculate partial FOVs for the projection (new version).
json calc_fov(const json& raw, const json& mesh, const harray2d_t* rot)
{
    std::unique_ptr<geom::Meshd> pHam;
    std::unique_ptr<geom::Rotation> pRot;

    if (!mesh.is_null()) {
        const harray2d_t verts = mesh[j_verts_opt];
        const hfaces_t faces = mesh[j_faces_opt];
        hedgelist_t edges = geom::faces_to_edges(faces);
        pHam = std::make_unique<geom::Meshd>(verts, edges);
    }
    if (nullptr != rot) {
        const auto rotMat
            = Eigen::Map<const Eigen::Matrix<double, 3, 3, Eigen::RowMajor>>(rot->data());
        pRot = std::make_unique<geom::Rotation>(rotMat);
    }
    auto frustum
        = geom::Frustum(raw[j_tan_left].get<double>(), raw[j_tan_right].get<double>(),
                        raw[j_tan_bottom].get<double>(), raw[j_tan_top].get<double>(),
                        pRot.get(), pHam.get());

    harray2d_t pts = frustum.get_fov_points(true);

    // calculate angles
    std::vector<double> deg_pts;
    const hvector_t base = {0, 0, -1};
    for (size_t i = 0, e = pts.shape(0); i < e; ++i) {
        const auto p = xt::view(pts, i);
        deg_pts.push_back(angle_deg(base, p));
    }

    json res;
    json fov_pts = pts;
    res[j_fov_pts] = fov_pts;
    res[j_deg_left] = -deg_pts[7];
    res[j_deg_right] = deg_pts[3];
    res[j_deg_bottom] = -deg_pts[1];
    res[j_deg_top] = deg_pts[5];
    res[j_deg_hor] = deg_pts[3] + deg_pts[7];
    res[j_deg_ver] = deg_pts[1] + deg_pts[5];

    return res;
}

//  Calculate total, vertical, horizontal and diagonal FOVs.
json calc_total_fov(const json& fov_head)
{
    // horizontal FOV
    const auto fov_hor = fov_head[j_reye][j_deg_right].get<double>()
        - fov_head[j_leye][j_deg_left].get<double>();

    // vertical FOV calculated from "straight ahead" look
    const auto ver_right = fov_head[j_reye][j_deg_top].get<double>()
        - fov_head[j_reye][j_deg_bottom].get<double>();
    const auto ver_left = fov_head[j_leye][j_deg_top].get<double>()
        - fov_head[j_leye][j_deg_bottom].get<double>();
    const auto fov_ver = (ver_left + ver_right) / 2.0;

    // diagonal FOV is calculated from diagonal FOV points:
    // [left_eye:left_bottom] <-> [right_eye:right_top]
    // and the other diagonal and is averaged over the two
    const hvector_t left_bottom = fov_head[j_leye][j_fov_pts][0];
    const hvector_t left_top = fov_head[j_leye][j_fov_pts][6];
    const hvector_t right_top = fov_head[j_reye][j_fov_pts][4];
    const hvector_t right_bottom = fov_head[j_reye][j_fov_pts][2];
    const auto diag1 = angle_deg(left_bottom, right_top);
    const auto diag2 = angle_deg(left_top, right_bottom);
    const auto fov_diag = (diag1 + diag2) / 2;

    // overlap
    const auto overlap = fov_head[j_leye][j_deg_right].get<double>()
        - fov_head[j_reye][j_deg_left].get<double>();

    return json({{j_fov_hor, fov_hor},
                 {j_fov_ver, fov_ver},
                 {j_fov_diag, fov_diag},
                 {j_overlap, overlap}});
}

//  Calculate the angle of the canted views and the IPD from eye to head transformation
//  matrices.
json calc_view_geom(const json& e2h)
{
    const harray2d_t left = e2h[j_leye];
    const harray2d_t right = e2h[j_reye];
    const auto cols = left.shape(1);

    // angle = acos(t dot v)/(|t|*|v|), where t = e2h * v
    // for v = [0, 0, -1] in eye coordinates => angle = acos(e2h[2,2])
    // direction e2h[0,2] > 0 => anti-clockwise, e2h[0,2] < 0 => clockwise
    // for simplicity use e2h[i, j] = e2h(i * cols + j)
    const auto left_rot
        = degrees(acos(left(2 * cols + 2)) * ((left(0 * cols + 2) > 0) ? -1.0 : 1.0));
    const auto right_rot
        = degrees(acos(right(2 * cols + 2)) * ((right(0 * cols + 2) > 0) ? -1.0 : 1.0));
    // IPD is stored in meters
    const auto ipd
        = point_dist(xt::view(left, xt::all(), 3), xt::view(right, xt::all(), 3));
    return json({{j_left_rot, left_rot}, {j_right_rot, right_rot}, {j_ipd, ipd}});
}

//  Calculate the additional data in the geometry data object (json)
json calc_geometry(const json& jd)
{
    json fov_eye;
    json fov_head;
    json ham_mesh;

    if (jd.contains(j_ham_mesh)) {
        ham_mesh = jd[j_ham_mesh];
    }
    else {
        ham_mesh = json::object({{j_leye, json()}, {j_reye, json()}});
    }

    for (const auto& neye : {j_leye, j_reye}) {

        // get eye to head transformation matrix
        const auto e2h = jd[j_eye2head][neye].get<harray2d_t>();

        // get raw eye values (direct from OpenVR)
        const auto raw_eye = jd[j_raw_eye][neye];

        // calculate optimized HAM mesh values
        if (!ham_mesh[neye].is_null()) {
            ham_mesh[neye] = calc_opt_ham_mesh(ham_mesh[neye]);
        }

        // build eye FOV points only if the eye FOV is rotated
        if (xt::view(e2h, xt::all(), xt::range(0, 3)) != xt::eye<double>(3, 0)) {
            fov_eye[neye] = calc_fov(raw_eye, ham_mesh[neye]);
        }

        // build head FOV points (they are eye FOV points if the views are parallel)
        harray2d_t rot = xt::view(e2h, xt::all(), xt::range(0, 3));
        fov_head[neye] = calc_fov(raw_eye, ham_mesh[neye], &rot);
    }

    // calculate total FOVs and the overlap
    auto fov_tot = calc_total_fov(fov_head);

    // calculate view rotation and the IPD
    auto view_geom = calc_view_geom(jd[j_eye2head]);

    // create a new object to ensure the right order of the newly inserted objects
    json res;
    // copy first only certain properties and only if they exist.
    // At the moment, it only concerns Oculus specific j_render_desc.
    for (const auto& name : {j_rec_rts, j_raw_eye, j_eye2head, j_render_desc}) {
        if (jd.contains(name)) {
            res[name] = jd[name];
        }
    }
    res[j_view_geom] = view_geom;
    res[j_fov_eye] = fov_eye;
    res[j_fov_head] = fov_head;
    res[j_fov_tot] = fov_tot;
    res[j_ham_mesh] = ham_mesh;

    return res;
}
