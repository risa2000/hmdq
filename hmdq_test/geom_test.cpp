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

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <xtensor/xbuilder.hpp>
#include <xtensor/xview.hpp>

#include "geom.h"

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this
                          // in one cpp file
#include <catch2/catch.hpp>

//  global setup
//------------------------------------------------------------------------------
// generic test samples
static const hvector_t o2 = {0, 0};
static const hvector_t a1 = {1, 0};
static const hvector_t a2 = {1, 1};
static const hvector_t a3 = {0, 1};
static const hvector_t a4 = {-1, 1};
static const hvector_t a5 = {-1, 0};
static const hvector_t a6 = {-1, -1};
static const hvector_t a7 = {0, -1};
static const hvector_t a8 = {1, -1};

static const hvector_t o3 = {0, 0, 0};
static const hvector_t b1 = {1, 0, 0};
static const hvector_t b2 = {1, 1, 1};
static const hvector_t b3 = {0, 1, 0};
static const hvector_t b4 = {-1, 1, -1};
static const hvector_t b5 = {-1, 0, 1};
static const hvector_t b6 = {-1, -1, -1};
static const hvector_t b7 = {0, -1, 0};
static const hvector_t b8 = {1, -1, 0};

static constexpr auto PI = xt::numeric_constants<double>::PI;

static harray2d_t mat1 = {{1, 2}, {3, 5}};
static harray2d_t mat2 = {{4, 10}, {2, 5}};
static harray2d_t mat3 = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

//  tests
//------------------------------------------------------------------------------
TEST_CASE("geometry module", "[geometry]")
{
    SECTION("radians to degrees", "[rad_to_deg]")
    {

        REQUIRE(degrees(PI) == 180);
        REQUIRE(degrees(0) == 0);
        REQUIRE(degrees(PI / 2) == 90);
    }

    SECTION("vector dot product", "[dot_prod]")
    {
        REQUIRE(dot_prod(a2, o2) == 0);
        REQUIRE(dot_prod(a1, a3) == 0);
        REQUIRE(dot_prod(a2, a6) == -2);
        REQUIRE(dot_prod(a4, a5) == 1);
        REQUIRE(dot_prod(a7, a8) == 1);

        REQUIRE(dot_prod(b1, b2) == 1);
        REQUIRE(dot_prod(b2, b2) == 3);
        REQUIRE(dot_prod(b6, b6) == 3);
        REQUIRE(dot_prod(b5, b4) == 0);
        REQUIRE(dot_prod(o3, b7) == 0);
    }

    SECTION("matrix determinant 2x2", "[mat_det_2x2]")
    {
        REQUIRE(det_mat_2x2(mat1) == -1);
        REQUIRE(det_mat_2x2(mat2) == 0);
        REQUIRE_THROWS_AS(det_mat_2x2(mat3), hmdq_exception);
    }

    SECTION("matrix determinant", "[mat_det]")
    {
        REQUIRE(det_mat(mat1 + mat2) == -10);
        REQUIRE(det_mat(mat1 - mat2) == 8);
        REQUIRE_THROWS_AS(det_mat(mat3), hmdq_exception);
    }

    SECTION("vector magnitude", "[norm]")
    {
        REQUIRE(norm(o2) == 0);
        REQUIRE(norm(a1) == 1);
        REQUIRE(norm(a2) == 1.4142135623730951);
        REQUIRE(norm(a4) == 1.4142135623730951);
        REQUIRE(norm(a7) == 1);
        REQUIRE(norm(a8) == 1.4142135623730951);
    }

    SECTION("vector angle", "[angle]")
    {
        REQUIRE(angle(a1, a2) == 0.7853981633974484);
        REQUIRE(std::isnan(angle(o2, a3)));
        REQUIRE(angle(a3, a3) == 0);
        REQUIRE(angle(b1, b2) == 0.9553166181245092);
        REQUIRE(angle(b4, b6) == 1.2309594173407747);
        REQUIRE(angle(b2, b6) == PI);
    }

    SECTION("vector angle degrees", "[angle_deg]")
    {
        REQUIRE(angle_deg(b7, b8) == Approx(45.0));
        REQUIRE(std::isnan(angle(o3, b1)));
        REQUIRE(angle_deg(b1, b3) == Approx(90.0));
        REQUIRE(angle_deg(b4, b5) == angle_deg(b5, b4));
        REQUIRE(angle(b2, b6) == angle(b6, b2));
        REQUIRE(angle(b1, b8) == angle(b8, b1));
    }

    SECTION("matrix multiplication", "[matmul]")
    {
        harray2d_t ident = xt::eye<double>(3, 0);
        harray2d_t mat1 = {{1, 2, 1}, {1, 0, 0}, {1, 1, 1}};
        harray2d_t mat2 = {{-4, 0, -1}, {1, -3, 3}, {-4, -2, -3}};
        harray2d_t mat3 = {{1, -1, -3}, {3, 0, -2}, {-4, -2, 0}};
        harray2d_t rmat1 = {{-6., -8., 2.}, {-4., 0., -1}, {-7., -5., -1.}};
        harray2d_t rmat2 = {{-5., -9., -5.}, {1., 5., 4.}, {-9., -11., -7.}};

        REQUIRE(bool(rmat1 == matmul(mat1, mat2)));
        REQUIRE(bool(rmat2 == matmul(mat2, mat1)));
        REQUIRE(bool(mat2 == matmul(mat2, ident)));
        REQUIRE(bool(mat2 == matmul(ident, mat2)));
        REQUIRE(bool(mat1 == matmul(mat1, ident)));
        REQUIRE(bool(mat1 == matmul(ident, mat1)));

        harray2d_t tmat1 = xt::view(mat1, xt::all(), xt::range(0, 2));
        harray2d_t tmat2 = xt::view(mat2, xt::range(0, 2), xt::all());
        harray2d_t rmat3 = {{-2, -6, 5}, {-4, 0, -1}, {-3, -3, 2}};

        REQUIRE(bool(rmat3 == matmul(tmat1, tmat2)));
        REQUIRE_THROWS_AS(matmul(tmat1, tmat1), hmdq_exception);
        REQUIRE_THROWS_AS(matmul(tmat2, tmat2), hmdq_exception);
    }

    SECTION("segment x segment intersection", "[seg_seg_int]")
    {
        REQUIRE(bool(seg_seg_int(a1, a2, a3, a4) == hvecpair_t({1, 1}, {0, 1})));
        REQUIRE(bool(seg_seg_int(o2, a2, o2, a4) == hvecpair_t({0, 0}, {0, 0})));
        REQUIRE(bool(seg_seg_int(o2, a2, o2, a4) == hvecpair_t({0, 0}, {0, 0})));
        REQUIRE(bool(seg_seg_int(a2, a6, a1, a5) == hvecpair_t({0, 0}, {0, 0})));
        REQUIRE(bool(seg_seg_int(a2, a3, a1, a2) == hvecpair_t(a2, a2)));
        REQUIRE(bool(seg_seg_int(a3, a8, a2, a8) == hvecpair_t(a8, a8)));
        REQUIRE(
            bool(seg_seg_int(o2, a6, a5, a7) == hvecpair_t({-0.5, -0.5}, {-0.5, -0.5})));
        REQUIRE(bool(seg_seg_int(a4, a5, a4, a6) == hvecpair_t(a4, a4)));

        hvecpair_t tres = seg_seg_int(a3, a6, o2, a4);
        auto tres2 = xt::stack(xt::xtuple(tres.first, tres.second));
        auto xres = hvector_t({-0.33333333333333337, 0.33333333333333326});
        auto xres2 = xt::stack(xt::xtuple(xres, xres));
        REQUIRE(0.0 == Approx(norm(tres2 - xres2)).margin(EPS));


        REQUIRE(bool(seg_seg_int(a4, a5, a4, a6) == hvecpair_t(a4, a4)));
        tres = seg_seg_int(a1, a2, a4, a5);
        REQUIRE(tres.first.size() == 0);
        REQUIRE(tres.second.size() == 0);
        tres = seg_seg_int(a2, a3, a6, a7);
        REQUIRE(tres.first.size() == 0);
        REQUIRE(tres.second.size() == 0);
        REQUIRE(bool(seg_seg_int(o2, a2, o2, a6) == hvecpair_t(o2, o2)));
    }

    SECTION("segment x mesh intersection", "[seg_mesh_int]")
    {
        harray2d_t verts = xt::stack(xt::xtuple(a5, a2, a4, a6, a8));
        std::vector<std::vector<size_t>> faces = {{0, 1, 2, 3, 4}};

        REQUIRE(bool(seg_mesh_int(o2, a5, verts, faces)
                     == xt::stack(xt::xtuple(a5, a5, a5))));
        REQUIRE(bool(seg_mesh_int(a7, a3, verts, faces)
                     == xt::stack(xt::xtuple(a3 / 2, a3, a7, a7 / 2))));
    }

    SECTION("find the closest point", "[find_closest]")
    {
        harray2d_t verts = xt::stack(xt::xtuple(a5, a2, a4, a6, a8));
        std::vector<std::vector<size_t>> faces = {{0, 1, 2, 3, 4}};

        auto tset = seg_mesh_int(o2, a5, verts, faces);
        REQUIRE(bool(find_closest(o2, tset) == a5));

        tset = seg_mesh_int(a7, a3, verts, faces);
        auto fset = find_closest(o2, tset);
        CAPTURE(fset);
        REQUIRE(bool(fset == a7 / 2 || fset == a3 / 2));

        tset = seg_mesh_int(a7 * 3, a3, verts, faces);
        REQUIRE(bool(find_closest(a7 * 3, tset) == a7));
    }

    SECTION("traingle surface calculation", "[area_triangle]")
    {
        REQUIRE(area_triangle(a1 * 3, o2, a3 * 4) == Approx(6.0));
        REQUIRE(area_triangle(o2, a3 * 4, a1 * 3) == Approx(6.0));
        REQUIRE(area_triangle(a3 * 4, a1 * 3, o2) == Approx(6.0));
        REQUIRE(area_triangle(a5 * 5, a6 * 5, o2) == Approx(25.0 / 2));
        REQUIRE(area_triangle(a5 * 5, a6 * 5, a7 * 5) == Approx(25.0 / 2));
        REQUIRE(area_triangle(a7, o2, a2) == Approx(1.0 / 2.0));
        REQUIRE(area_triangle(o2, a3, a2) == Approx(1.0 / 2.0));
        REQUIRE(area_triangle(o2, o2, a2) == Approx(0));
        REQUIRE(area_triangle(a2, o2, o2) == Approx(0));
        REQUIRE(area_triangle(o2, a2, o2) == Approx(0));
        REQUIRE(area_triangle(a5, a1, a7 * 2) == Approx(2.0));
    }

    SECTION("triangle mesh surface calculation", "[area_mesh]")
    {
        harray2d_t verts
            = xt::stack(xt::xtuple(o2, a1, a2, o2, a2, a3, o2, a3, a4, o2, a4, a5));
        harray2d_t verts2
            = xt::stack(xt::xtuple(o2, a5, a6, o2, a6, a7, o2, a7, a8, o2, a8, a1));
        REQUIRE(area_mesh(verts) == Approx(2.0));
        REQUIRE(area_mesh(verts2) == Approx(2.0));
    }
}
