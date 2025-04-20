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

#include <common/except.h>
#include <common/xtdef.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/StdVector>

#include <vector>

namespace geom {

//  globals
//------------------------------------------------------------------------------
constexpr double DOUBLE_EPS_100 = std::numeric_limits<double>::epsilon() * 100;
constexpr double DOUBLE_MAX = std::numeric_limits<double>::max();

//  typedefs
//------------------------------------------------------------------------------
using Point3 = Eigen::Vector<double, 3>;
using Plane = Eigen::Hyperplane<double, 3>;
using Line = Eigen::ParametrizedLine<double, 3>;
using Rotation = Eigen::AngleAxis<double>;
using Transform2 = Eigen::Transform<double, 2, Eigen::Affine, Eigen::RowMajor>;
using Point3Array = std::vector<Point3, Eigen::aligned_allocator<Point3>>;
using PlaneArray = std::vector<Plane>;

//  helper functions
//------------------------------------------------------------------------------
//  Convert faces to edges
hedgelist_t faces_to_edges(const hfaces_t& faces);
//  Calculate point "polarity" to the plane (whether it is above, below, or in the plane)
int polarity(const Plane& plane, const Point3& point);

//  class Meshd
//------------------------------------------------------------------------------
//  Calculates the frustum FOV while incorporating the hidden area mesh (HAM) if present.
class Meshd
{
  public:
    Meshd() = default;
    Meshd(const Meshd& mesh) = default;
    Meshd(const harray2d_t& verts, const hedgelist_t& edges)
        : m_edges(edges)
        , m_verts(verts)
    {}

    //  Add another mesh to this to form one mesh
    void add_mesh(const harray2d_t& verts, const hedgelist_t& edges);

    const harray2d_t& get_verts() const
    {
        return m_verts;
    }

    const hedgelist_t& get_edges() const
    {
        return m_edges;
    }

  private:
    harray2d_t m_verts;
    hedgelist_t m_edges;
};

//  class Frustum
//------------------------------------------------------------------------------
//  Calculates the frustum FOV while incorporating the hidden area mesh (HAM) if present.
class Frustum
{
  public:
    Frustum(double left, double right, double bottom, double top,
            const Rotation* pRot = nullptr, const Meshd* pHam = nullptr);

    //  Calculate all FOV points and put them into one array (LB, B, RB, ..)
    harray2d_t get_fov_points(bool projected = false);

  private:
    //  Construct plane which cuts the frustum through center, given point and is aligned
    //  with the view direction (vector forward).
    Plane get_point_plane(const Point3& point)
    {
        return Plane::Through(m_center, point, m_forward);
    }

    //  Create transformation from UV space into frustum LRBT rectangle
    Transform2 get_uv_to_lrbt_transform()
    {
        Transform2 res = Eigen::Translation2d(m_leftTan, m_bottomTan)
            * Eigen::Scaling(m_rightTan - m_leftTan, m_topTan - m_bottomTan);
        return res;
    }

    //  Build 3D representation of the HAM inside the frustum from 2D UV definition
    void get_ham_3d(const Meshd& ham2d, Meshd& ham3d);

    //  Calculate one "raw" FOV point as an intersection of a "point-plane" and the HAM
    Point3 get_raw_fov_point(size_t n);

  private:
    double m_leftTan;
    double m_rightTan;
    double m_bottomTan;
    double m_topTan;

    Point3 m_center;
    Point3 m_forward;
    Point3 m_leftBottom;
    Point3 m_bottom;
    Point3 m_rightBottom;
    Point3 m_right;
    Point3 m_rightTop;
    Point3 m_top;
    Point3 m_leftTop;
    Point3 m_left;

    const Rotation* m_pRot;
    Meshd m_ham3d;

    Point3Array m_outPoints;
    PlaneArray m_pointPlanes;

    std::vector<int> m_polarityPlaneIndexes;
};

} // namespace geom
