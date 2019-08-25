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

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include <xtensor/xarray.hpp>
#include <xtensor/xbuilder.hpp>
#include <xtensor/xjson.hpp>
#include <xtensor/xview.hpp>

#include "geom.h"
#include "hmdview.h"
#include "jtools.h"
#include "xtdef.h"

#include "fifo_map_fix.h"

//  functions
//------------------------------------------------------------------------------
//  Transform vertices from UV space into LRBT space.
harray2d_t verts_uv2lrbt(const harray2d_t& verts, double l, double r, double b, double t)
{
    const auto trans = uv2lrbt(l, r, b, t);
    // add ones into the third "dimension" of the 2D points to acknowledge the translation
    const xt::xtensor<double, 2>::shape_type shape = {1, verts.shape(0)};
    const harray2d_t tones = xt::ones<double>(shape);
    // nverts end up being transposed, which is useful for the matmul later
    const harray2d_t nverts = xt::concatenate(xt::xtuple(xt::transpose(verts), tones));
    // trasform 2D points in `nverts` by using `trans` transformation matrix
    const auto tres = matmul(trans, nverts);
    // we need to transpose it back so the points are the rows of the array
    return xt::transpose(tres);
}

//  Build 2D points/vectors (depends on `bpt`) for LRBT rectangle.
harray2d_t build_lrbt_quad_2d(const json& raw, double norm)
{
    const auto l = raw["tan_left"].get<double>() * norm;
    const auto r = raw["tan_right"].get<double>() * norm;
    const auto b = raw["tan_bottom"].get<double>() * norm;
    const auto t = raw["tan_top"].get<double>() * norm;
    return harray2d_t({{l, b}, {r, b}, {r, t}, {l, t}});
}

//  Calculate partial FOVs for the projection.
json get_fov(const json& raw, const json& mesh, const harray2d_t* rot)
{
    harray2d_t verts2d;
    hfaces_t mfaces;
    if (!mesh.is_null()) {
        // if there is a mesh, stretch it to LRBT quad, ortherwise use just the quad
        const harray2d_t uvverts = mesh["verts_opt"];
        mfaces = mesh["faces_opt"].get<hfaces_t>();
        verts2d = verts_uv2lrbt(
            uvverts, raw["tan_left"].get<double>(), raw["tan_right"].get<double>(),
            raw["tan_bottom"].get<double>(), raw["tan_top"].get<double>());
        verts2d = xt::concatenate(xt::xtuple(verts2d, build_lrbt_quad_2d(raw)));
    }
    else {
        verts2d = build_lrbt_quad_2d(raw);
    }
    // now verts hold all mesh vertices plus LRBT base quad vertices
    auto len = verts2d.shape(0);
    const hface_t lrbt_face = {len - 4, len - 3, len - 2, len - 1};
    // add the face for the LRBT quad
    mfaces.push_back(lrbt_face);
    // put everything into 3D (at z=-1.0)
    xt::xtensor<double, 2>::shape_type shape = {1, verts2d.shape(0)};
    harray2d_t tones = xt::ones<double>(shape);
    harray2d_t verts3d = xt::transpose(
        xt::concatenate(xt::xtuple(xt::transpose(verts2d), tones * -1.0)));
    if (nullptr != rot) {
        // rotate the projection plane quad
        const auto tverts3d = matmul(verts3d, xt::transpose(*rot));
        // get the last column and use it to normalize the points
        const auto tcol = xt::view(tverts3d, xt::all(), 2);
        const auto zcol = xt::expand_dims(tcol, 1);
        // project the quad into z=-1.0 plane (again)
        verts3d = tverts3d / abs(zcol);
        // verts3d changed, update verts2d (get only 2D coordinates, z must be -1.0)
        verts2d = xt::view(verts3d, xt::all(), xt::range(0, 2));
    }
    // calculate FOV points (corners and points on the edges with one coordinate==0)
    // make sure the "mid-edge" segments overreach the original dimensions
    const double fac = 1.1;
    len = verts2d.shape(0);
    const auto lb = xt::view(verts2d, len - 4);
    const hvector_t bc
        = {0,
           std::min(xt::view(verts2d, len - 4, 1)[0], xt::view(verts2d, len - 3, 1)[0])
               * fac};
    const auto rb = xt::view(verts2d, len - 3);
    const hvector_t rc
        = {std::max(xt::view(verts2d, len - 3, 0)[0], xt::view(verts2d, len - 2, 0)[0])
               * fac,
           0};
    const auto rt = xt::view(verts2d, len - 2);
    const hvector_t tc
        = {0,
           std::max(xt::view(verts2d, len - 2, 1)[0], xt::view(verts2d, len - 1, 1)[0])
               * fac};
    const auto lt = xt::view(verts2d, len - 1);
    const hvector_t lc
        = {std::min(xt::view(verts2d, len - 1, 0)[0], xt::view(verts2d, len - 4, 0)[0])
               * fac,
           0};
    const harray2d_t vectors = xt::stack(xt::xtuple(lb, bc, rb, rc, rt, tc, lt, lc));

    hveclist_t pts;
    const hvector_t origin = {0, 0};
    for (size_t i = 0, e = vectors.shape(0); i < e; ++i) {
        const auto v = xt::view(vectors, i);
        const auto tpts = seg_mesh_int(origin, v, verts2d, mfaces);
        HMDQ_ASSERT(tpts.size());
        pts.push_back(find_closest(origin, tpts));
    }

    const auto apts = build_array(pts);
    // add z-coord at z = -1.0 to make them 3D points again
    shape = {1, pts.size()};
    tones = xt::ones<double>(shape);
    const harray2d_t pts3d
        = xt::transpose(xt::concatenate(xt::xtuple(xt::transpose(apts), tones * -1.0)));
    // calculate angles
    std::vector<double> deg_pts;
    const hvector_t base = {0, 0, -1};
    for (size_t i = 0, e = pts3d.shape(0); i < e; ++i) {
        const auto p = xt::view(pts3d, i);
        deg_pts.push_back(angle_deg(base, p));
    }

    json res;
    json fov_pts = pts3d;
    res["fov_pts"] = fov_pts;
    res["deg_left"] = -deg_pts[7];
    res["deg_right"] = deg_pts[3];
    res["deg_bottom"] = -deg_pts[1];
    res["deg_top"] = deg_pts[5];
    res["deg_hor"] = deg_pts[3] + deg_pts[7];
    res["deg_ver"] = deg_pts[1] + deg_pts[5];

    return res;
}

//  Calculate total, vertical, horizontal and diagonal FOVs.
json get_total_fov(const json& fov_head)
{
    // horizontal FOV
    const auto fov_hor = fov_head[REYE]["deg_right"].get<double>()
        - fov_head[LEYE]["deg_left"].get<double>();

    // vertical FOV calculated from "straight ahead" look
    const auto fov_ver = std::min(fov_head[REYE]["deg_top"].get<double>(),
                                  fov_head[LEYE]["deg_top"].get<double>())
        - std::max(fov_head[REYE]["deg_bottom"].get<double>(),
                   fov_head[LEYE]["deg_bottom"].get<double>());

    // diagonal FOV is calculated from diagonal FOV points:
    // [left_eye:left_bottom] <-> [right_eye:right_top]
    // and the other diagonal and is averaged over the two
    const hvector_t left_bottom = fov_head[LEYE]["fov_pts"][0];
    const hvector_t left_top = fov_head[LEYE]["fov_pts"][6];
    const hvector_t right_top = fov_head[REYE]["fov_pts"][4];
    const hvector_t right_bottom = fov_head[REYE]["fov_pts"][2];
    const auto diag1 = angle_deg(left_bottom, right_top);
    const auto diag2 = angle_deg(left_top, right_bottom);
    const auto fov_diag = (diag1 + diag2) / 2;

    // overlap
    const auto overlap = fov_head[LEYE]["deg_right"].get<double>()
        - fov_head[REYE]["deg_left"].get<double>();

    return json({{"fov_hor", fov_hor},
                 {"fov_ver", fov_ver},
                 {"fov_diag", fov_diag},
                 {"overlap", overlap}});
}

//  Calculate the angle of the canted views and the IPD from eye to head transformation
//  matrices.
json get_view_geom(const json& e2h)
{
    const harray2d_t left = e2h[LEYE];
    const harray2d_t right = e2h[REYE];
    const auto cols = left.shape(1);

    // angle = acos(t dot v)/(|t|*|v|), where t = e2h * v
    // for v = [0, 0, -1] in eye coordinates => angle = acos(e2h[2,2])
    // direction e2h[0,2] > 0 => anti-clockwise, e2h[0,2] < 0 => clockwise
    // for simplicity use e2h[i, j] = e2h(i * cols + j)
    const auto left_rot
        = degrees(acos(left(2 * cols + 2)) * ((left(0 * cols + 2) > 0) ? -1.0 : 1.0));
    const auto right_rot
        = degrees(acos(right(2 * cols + 2)) * ((right(0 * cols + 2) > 0) ? -1.0 : 1.0));
    // IPD is reported in mm
    const auto ipd
        = point_dist(xt::view(left, xt::all(), 3), xt::view(right, xt::all(), 3)) * 1000;
    return json({{"left_rot", left_rot}, {"right_rot", right_rot}, {"ipd", ipd}});
}
