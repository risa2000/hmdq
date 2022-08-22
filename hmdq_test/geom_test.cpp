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

#include <xtensor/xbuilder.hpp>
#include <xtensor/xview.hpp>

#include "geom.h"

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this
                          // in one cpp file
#include <catch2/catch_all.hpp>

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

    SECTION("vector magnitude", "[norm]")
    {
        REQUIRE(gnorm(o2) == 0.0);
        REQUIRE(gnorm(a1) == 1.0);
        REQUIRE(gnorm(a2) == 1.4142135623730951);
        REQUIRE(gnorm(a4) == 1.4142135623730951);
        REQUIRE(gnorm(a7) == 1.0);
        REQUIRE(gnorm(a8) == 1.4142135623730951);
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
        using Approx = Catch::Approx;
        REQUIRE(angle_deg(b7, b8) == Approx(45.0));
        REQUIRE(std::isnan(angle(o3, b1)));
        REQUIRE(angle_deg(b1, b3) == Approx(90.0));
        REQUIRE(angle_deg(b4, b5) == angle_deg(b5, b4));
        REQUIRE(angle(b2, b6) == angle(b6, b2));
        REQUIRE(angle(b1, b8) == angle(b8, b1));
    }

    SECTION("traingle surface calculation", "[area_triangle]")
    {
        using Approx = Catch::Approx;
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
}
