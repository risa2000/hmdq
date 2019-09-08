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

#include "verhlp.h"

// Already defined in geom_test.cpp
// #define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

//  tests
//------------------------------------------------------------------------------
TEST_CASE("Version string comparison module", "[verhlp]")
{
    SECTION("naive test", "[naive]")
    {
        REQUIRE(comp_ver("1", "1") == 0);
        REQUIRE(comp_ver("1", "1") == 0);
        REQUIRE(comp_ver("1", "1.0") == 0);
        REQUIRE(comp_ver("1.0.0", "1.0") == 0);
        REQUIRE(comp_ver("1.0.0", "1.0.0.0") == 0);
    }

    SECTION("missing test", "[missing]")
    {
        REQUIRE(comp_ver(".", ".") == 0);
        REQUIRE(comp_ver(".", "0.") == 0);
        REQUIRE(comp_ver(".", ".0") == 0);
        REQUIRE(comp_ver(".0", "0.") == 0);
        REQUIRE(comp_ver(".1", "0.1") == 0);
        REQUIRE(comp_ver("1.", "1") == 0);
    }

    SECTION("length test", "[length]")
    {
        REQUIRE(comp_ver("1.0", "1.0.1") == -1);
        REQUIRE(comp_ver("1.0", "1.0.1.0") == -1);
        REQUIRE(comp_ver("1.0.0.1", "1.0.0.001.") == 0);
        REQUIRE(comp_ver("1.0.1", "1.0") == 1);
        REQUIRE(comp_ver("1.0.1.0", "1.0") == 1);
        REQUIRE(comp_ver("1.0.0.0.1", "1.0") == 1);
    }

    SECTION("main test", "[main]")
    {
        REQUIRE(comp_ver("1.2.3", "1.2.2") == 1);
        REQUIRE(comp_ver("1.2.1", "1.2.2") == -1);
        REQUIRE(comp_ver("1.2.1", "1.1.2") == 1);
        REQUIRE(comp_ver("1.0.1", "1.1.2") == -1);
        REQUIRE(comp_ver("1.0", "1.1.2") == -1);
        REQUIRE(comp_ver("1", "1.1.2") == -1);
        REQUIRE(comp_ver("1.0.1", "1.1") == -1);
        REQUIRE(comp_ver("1.0.1", "1") == 1);
    }
}
