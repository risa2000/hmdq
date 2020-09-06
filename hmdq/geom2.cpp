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

#include "geom2.h"

#include <xtensor/xadapt.hpp>
#include <xtensor/xindex_view.hpp>
#include <xtensor/xview.hpp>

namespace geom {

//  locals
//------------------------------------------------------------------------------
constexpr double DOUBLE_EPS_100 = std::numeric_limits<double>::epsilon() * 100;
constexpr double DOUBLE_MAX = std::numeric_limits<double>::max();

//  helper functions
//------------------------------------------------------------------------------
//  Convert faces to edges
hedgelist_t faces_to_edges(const hfaces_t& faces)
{
    hedgelist_t edges;
    for (const auto& f : faces) {
        size_t e = f.size();
        for (size_t i = 0; i < e; ++i) {
            edges.emplace_back<hedge_t>({f[i], f[(i + 1) % e]});
        }
    }
    return edges;
}

//  Calculate point "polarity" to the plane (whether it is above, below, or in the plane)
int polarity(const Plane& plane, const Point3& point)
{
    double dist = plane.signedDistance(point);
    if (abs(dist) <= DOUBLE_EPS_100) {
        return 0;
    }
    else {
        return signbit(dist) ? -1 : 1;
    }
}

//  class Meshd
//------------------------------------------------------------------------------

//  Add another mesh to this to form one mesh
void Meshd::add_mesh(const harray2d_t& verts, const hedgelist_t& edges)
{
    //  if the members are not initialized, just initialize them
    if (0 == m_verts.shape(0)) {
        m_verts = verts;
        m_edges = edges;
    }
    else {
        auto vcount = m_verts.shape(0);
        //  add the vertices at the end
        m_verts = xt::concatenate(xt::xtuple(m_verts, verts));
        for (const auto& e : edges) {
            m_edges.emplace_back<hedge_t>({e.first + vcount, e.second + vcount});
        }
    }
}

//  class Frustum
//------------------------------------------------------------------------------
//  Calculates the frustum FOV while incorporating the hidden area mesh (HAM) if present.
Frustum::Frustum(double left, double right, double bottom, double top,
                 const Rotation* pRot, const Meshd* pHam)
    : m_leftTan(left), m_rightTan(right), m_bottomTan(bottom), m_topTan(top),
      m_center(0, 0, 0), m_forward(0, 0, -1), m_leftBottom(left, bottom, -1),
      m_bottom(0, -1, -1), m_rightBottom(right, bottom, -1), m_right(1, 0, -1),
      m_rightTop(right, top, -1), m_top(0, 1, -1), m_leftTop(left, top, -1),
      m_left(-1, 0, -1), m_pRot(pRot)
{
    if (nullptr != m_pRot) {
        m_leftBottom = *m_pRot * m_leftBottom;
        m_rightBottom = *m_pRot * m_rightBottom;
        m_rightTop = *m_pRot * m_rightTop;
        m_leftTop = *m_pRot * m_leftTop;
    }

    m_outPoints = {m_leftBottom, m_bottom, m_rightBottom, m_right,
                   m_rightTop,   m_top,    m_leftTop,     m_left};
    m_polarityPlaneIndexes = {3, 3, 3, 1, 3, 3, 3, 1};

    //  add default LRBT rectangle to the HAM to ensure the FOV points will be found
    //  in case the HAM does not cover all LRBT rectangle edges.
    harray2d_t lrbt_verts = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    hedgelist_t lrbt_edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
    std::unique_ptr<Meshd> tHam2d;
    if (nullptr == pHam) {
        tHam2d = std::make_unique<Meshd>(lrbt_verts, lrbt_edges);
    }
    else {
        tHam2d = std::make_unique<Meshd>(*pHam);
        tHam2d->add_mesh(lrbt_verts, lrbt_edges);
    }

    get_ham_3d(*tHam2d, m_ham3d);

    for (const auto& point : m_outPoints) {
        m_pointPlanes.push_back(get_point_plane(point));
    }
}

//  Calculate all FOV points and put them into one array (LB, B, RB, ..)
harray2d_t Frustum::get_fov_points(bool projected)
{
    const size_t ptcount = m_outPoints.size();
    harray2d_t::shape_type shape = {ptcount, 3};
    harray2d_t points(shape);
    for (size_t i = 0; i < ptcount; ++i) {
        auto pt = get_raw_fov_point(i);
        points(i, 0) = pt.x();
        points(i, 1) = pt.y();
        points(i, 2) = pt.z();
    }
    //  sanitization
    xt::filter(points, abs(points) < DOUBLE_EPS_100) = 0.0;
    if (projected) {
        // get the last column and use it to normalize the points
        const auto tcol = xt::view(points, xt::all(), 2);
        const auto zcol = xt::expand_dims(tcol, 1);
        // project the points onto z=+/-1.0 plane (again) to get "normalized" frustum
        // points
        harray2d_t tpoints = points / abs(zcol);
        return tpoints;
    }
    else {
        return points;
    }
}

//  Build 3D representation of the HAM inside the frustum from 2D UV definition
void Frustum::get_ham_3d(const Meshd& ham2d, Meshd& ham3d)
{
    const auto& edges2d = ham2d.get_edges();
    const auto& verts2d = ham2d.get_verts();

    // add ones into the third "dimension" of the 2D points to acknowledge the
    // translation
    const size_t vcount = verts2d.shape(0);
    xt::xtensor<double, 2>::shape_type shape = {vcount, 1};
    const harray2d_t tones = xt::ones<double>(shape);
    // nverts end up being transposed, which is useful for the matmul later
    harray2d_t verts2d_h = xt::hstack(xt::xtuple(verts2d, tones));
    auto verts2d_h_e
        = Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor>>(
            verts2d_h.data(), vcount, 3);
    auto uv2lrbt = get_uv_to_lrbt_transform();
    Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor> verts_trans
        = (verts2d_h_e * uv2lrbt.matrix().transpose()).eval();
    //  "rename" homogeneous coordinate to Z-coord and set it to -1.0 where the
    //  frustum projection plane is constructed.
    verts_trans.col(2).setConstant(-1.0);

    if (nullptr != m_pRot) {
        auto verts_rot = verts_trans * m_pRot->matrix().transpose();
        verts_trans = verts_rot;
    }
    shape = {vcount, 3};
    harray2d_t verts3d_rot
        = xt::adapt(verts_trans.data(), vcount * 3, xt::no_ownership(), shape);
    ham3d.add_mesh(verts3d_rot, edges2d);
}

//  Calculate one "raw" FOV point as an intersection of a "point-plane" and the HAM
Point3 Frustum::get_raw_fov_point(size_t n)
{
    const Plane& polPlane = m_pointPlanes[m_polarityPlaneIndexes[n]];
    const Plane& cutPlane = m_pointPlanes[n];
    int outPointPol = polarity(polPlane, m_outPoints[n]);
    double dmin = DOUBLE_MAX;
    Point3 point;
    bool found = false;
    const harray2d_t& verts = m_ham3d.get_verts();
    for (const auto& edge : m_ham3d.get_edges()) {
        auto xpt1 = xt::view(verts, edge.first);
        auto xpt2 = xt::view(verts, edge.second);
        Point3 pt1 = Point3(xpt1(0), xpt1(1), xpt1(2));
        Point3 pt2 = Point3(xpt2(0), xpt2(1), xpt2(2));
        double limit = (pt2 - pt1).norm();
        const Line line = Line::Through(pt1, pt2);
        double t = line.intersectionParameter(cutPlane);
        if (t >= -DOUBLE_EPS_100 && t <= limit + DOUBLE_EPS_100) {
            Point3 pt = line.pointAt(t);
            int ptPol = polarity(polPlane, pt);
            if (ptPol == outPointPol) {
                double dist = (m_center - pt).norm();
                if (dist < dmin) {
                    dmin = dist;
                    point = pt;
                    found = true;
                }
            }
        }
    }
    HMDQ_ASSERT(found);
    return point;
}

} // namespace geom
