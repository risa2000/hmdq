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
#include "optmesh.h"

// Already defined in geom_test.cpp
// #define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

//  global setup
//------------------------------------------------------------------------------
const hface_t f1 = {1, 2, 3, 4, 5};
const hedgelist_t e1 = {{1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 1}};

const hface_t f2 = {4, 5, 10, 3};
const hedgelist_t e2 = {{4, 5}, {5, 10}, {10, 3}, {3, 4}};
const hedgelist_t se2 = {{4, 5}, {5, 10}, {3, 10}, {3, 4}};
const hedgelist_t re2 = {{4, 3}, {3, 10}, {10, 5}, {5, 4}};

const hedgelist_t se1e2 = {{3, 4}, {4, 5}};

const hvector_t v1 = {1.0, 2.0, 3.0};
const hvector_t v2 = {4.0, 5.0, 6.0};
const hvector_t v3 = {7.0, 8.0, 9.0};
const hvector_t v4 = {0.0, 1.0, 1.0};
const hvector_t v5 = {-0.5, -1.0, 100};

const hveclist_t verts = {v1, v2, v4, v5};

const harray2d_t verts1
    = xt::stack(xt::xtuple(v1, v2, v3, v1, v3, v4, v1, v4, v5, v1, v5, v2));
// build trivial triangle faces for verts1
const hfaces_t faces1 = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {9, 10, 11}};

const harray2d_t verts2 = xt::stack(xt::xtuple(v1, v2, v3, v4, v5));
const hfaces_t faces2 = {{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1}};

const hfaces_t faces3 = {{0, 1, 2}, {0, 3, 2}, {0, 4, 3}, {0, 1, 4}};
const std::vector<hedgelist_t> edges3 = {{{0, 1}, {1, 2}, {2, 0}},
                                         {{0, 3}, {3, 2}, {2, 0}},
                                         {{0, 4}, {4, 3}, {3, 0}},
                                         {{0, 1}, {1, 4}, {4, 0}}};

//  tests
//------------------------------------------------------------------------------
TEST_CASE("HAM mesh optimization module", "[optmesh]")
{
    SECTION("vertex in vertices", "[v_in_verts]")
    {
        REQUIRE(v_in_verts(v1, verts) == 0);
        REQUIRE(v_in_verts(v2, verts) == 1);
        REQUIRE(v_in_verts(v3, verts) == -1);
        REQUIRE(v_in_verts(v4, verts) == 2);
        REQUIRE(v_in_verts(v5, verts) == 3);
    }

    SECTION("eliminate duplicate vertices", "[reduce_verts]")
    {
        auto [tverts2, tfaces2] = reduce_verts(verts1, faces1);
        REQUIRE(bool(tverts2 == verts2));
        REQUIRE(tfaces2 == faces2);
    }

    SECTION("convert faces to edges", "[face2edges]")
    {
        REQUIRE(e1 == face2edges(f1));
        REQUIRE(e2 == face2edges(f2));
        for (size_t i = 0, e = faces3.size(); i < e; ++i) {
            REQUIRE(edges3[i] == face2edges(faces3[i]));
        }
    }

    SECTION("sort individual edges", "[sort_edges]") { REQUIRE(se2 == sort_edges(e2)); }

    SECTION("find shared edges", "[shared_edges]")
    {
        REQUIRE(se1e2 == shared_edges(e1, e2));
        REQUIRE(hedgelist_t({{0, 2}}) == shared_edges(edges3[0], edges3[1]));
        REQUIRE(hedgelist_t({{0, 3}}) == shared_edges(edges3[1], edges3[2]));
        REQUIRE(hedgelist_t({{0, 4}}) == shared_edges(edges3[2], edges3[3]));
        REQUIRE(hedgelist_t({{0, 1}}) == shared_edges(edges3[3], edges3[0]));
        REQUIRE(hedgelist_t() == shared_edges(edges3[0], edges3[2]));
        REQUIRE(hedgelist_t() == shared_edges(edges3[1], edges3[3]));
    }

    SECTION("reverse the order and the edges as well", "[reverse_edges]")
    {
        REQUIRE(re2 == reverse_edges(e2));
        REQUIRE(hedgelist_t({{1, 2}}) == reverse_edges(hedgelist_t({{2, 1}})));
        REQUIRE(hedgelist_t({{1, 1}}) == reverse_edges(hedgelist_t({{1, 1}})));
    }

    SECTION("build face from two edge lists", "[build_faces]")
    {
        REQUIRE(hface_t({1, 2, 3})
                == build_face(hedgelist_t({{1, 2}, {2, 3}}), hedgelist_t({{3, 1}})));
        REQUIRE(hface_t({1, 2, 3})
                == build_face(hedgelist_t({{1, 2}, {2, 3}}), hedgelist_t({{1, 3}})));
        REQUIRE(hface_t({3, 2, 1})
                == build_face(hedgelist_t({{3, 2}, {2, 1}}), hedgelist_t({{3, 1}})));
        REQUIRE(hface_t({3, 2, 1})
                == build_face(hedgelist_t({{3, 2}, {2, 1}}), hedgelist_t({{1, 3}})));
    }

    SECTION("match edges regardless the direction", "[match_edges]")
    {
        REQUIRE(false == match_edges(hedge_t(3, 4), hedge_t(0, 1)));
        REQUIRE(false == match_edges(hedge_t(3, 4), hedge_t(2, 3)));
        REQUIRE(false == match_edges(hedge_t(3, 4), hedge_t(3, 1)));
        REQUIRE(true == match_edges(hedge_t(3, 4), hedge_t(3, 4)));
        REQUIRE(true == match_edges(hedge_t(4, 3), hedge_t(4, 3)));
    }

    SECTION("remove continuous chain from the edge list", "[remove_chain]")
    {
        REQUIRE(hedgelist_t({{0, 1}, {1, 2}})
                == remove_chain(hedgelist_t({{0, 2}}),
                                hedgelist_t({{0, 1}, {1, 2}, {2, 0}})));
        REQUIRE(hedgelist_t({{0, 3}, {3, 2}})
                == remove_chain(hedgelist_t({{0, 2}}),
                                hedgelist_t({{0, 3}, {3, 2}, {2, 0}})));
        auto res1 = hedgelist_t({{4, 5}, {5, 0}, {0, 1}});
        auto res2 = hedgelist_t({{4, 7}, {7, 6}, {6, 1}});
        auto res3 = hedgelist_t({{1, 0}, {0, 5}, {5, 4}});
        auto res4 = hedgelist_t({{1, 6}, {6, 7}, {7, 4}});
        auto chain = hedgelist_t({{3, 4}, {2, 3}, {1, 2}});
        REQUIRE(
            res1
            == remove_chain(
                chain, hedgelist_t({{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 0}})));
        REQUIRE(
            res2
            == remove_chain(
                chain, hedgelist_t({{6, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 7}, {7, 6}})));
        REQUIRE(
            res1
            == remove_chain(
                chain, hedgelist_t({{3, 4}, {4, 5}, {5, 0}, {0, 1}, {1, 2}, {2, 3}})));
        REQUIRE(
            res3
            == remove_chain(
                chain, hedgelist_t({{5, 4}, {4, 3}, {3, 2}, {2, 1}, {1, 0}, {0, 5}})));
        REQUIRE(
            res4
            == remove_chain(
                chain, hedgelist_t({{7, 4}, {4, 3}, {3, 2}, {2, 1}, {1, 6}, {6, 7}})));
        REQUIRE(
            res3
            == remove_chain(
                chain, hedgelist_t({{2, 1}, {1, 0}, {0, 5}, {5, 4}, {4, 3}, {3, 2}})));
    }

    SECTION("merging two edge lists with shared edge(s)", "[merge_edges]")
    {
        REQUIRE(hface_t({0, 1, 2, 3})
                == merge_edges(edges3[0], edges3[1], hedgelist_t({{0, 2}})));
        REQUIRE(hface_t({3, 2, 0, 4})
                == merge_edges(edges3[1], edges3[2], hedgelist_t({{0, 3}})));
        REQUIRE(hface_t({4, 3, 0, 1})
                == merge_edges(edges3[2], edges3[3], hedgelist_t({{0, 4}})));
        REQUIRE(hface_t({1, 2, 0, 4})
                == merge_edges(edges3[0], edges3[3], hedgelist_t({{0, 1}})));

        REQUIRE_THROWS_AS(merge_edges(edges3[0], edges3[1], hedgelist_t({{0, 1}})),
                          hmdq_exception);
        REQUIRE_THROWS_AS(merge_edges(edges3[1], edges3[2], hedgelist_t({{0, 2}})),
                          hmdq_exception);
        REQUIRE_THROWS_AS(merge_edges(edges3[2], edges3[3], hedgelist_t({{4, 3}})),
                          hmdq_exception);

        REQUIRE(hface_t({4, 5, 1, 2, 3, 10, 5})
                == merge_edges(e1, e2, hedgelist_t({{3, 4}})));
        REQUIRE(hface_t({5, 1, 2, 3, 4, 3, 10})
                == merge_edges(e1, e2, hedgelist_t({{5, 4}})));
    }

    SECTION("check if edges in the list are chained", "[check_chained]")
    {
        REQUIRE(hedgelist_t({{3, 4}}) == check_chained(hedgelist_t({{3, 4}})));
        REQUIRE(hedgelist_t({{1, 3}, {3, 4}})
                == check_chained(hedgelist_t({{1, 3}, {3, 4}})));
        REQUIRE(hedgelist_t({{3, 4}, {2, 3}, {1, 2}})
                == check_chained(hedgelist_t({{1, 2}, {3, 4}, {2, 3}})));
        REQUIRE(hedgelist_t({{3, 4}, {2, 3}, {1, 2}})
                == check_chained(hedgelist_t({{3, 4}, {2, 3}, {1, 2}})));
        REQUIRE(hedgelist_t({{3, 4}, {2, 3}, {1, 2}})
                == check_chained(hedgelist_t({{2, 3}, {1, 2}, {3, 4}})));
        REQUIRE(hedgelist_t({}) == check_chained(hedgelist_t({{1, 2}, {3, 4}})));
    }

    SECTION("reduce faces in the mesh by merging them", "[reduce_faces]")
    {
        REQUIRE(hfaces_t({{5, 1, 2, 3, 10}}) == reduce_faces(hfaces_t({f1, f2})));
        REQUIRE(faces1 == reduce_faces(faces1));

        hface_t f3 = {0, 1, 2, 3, 4, 5};
        hface_t f3s = {3, 4, 5, 0, 1, 2};
        hface_t f4 = {6, 1, 2, 3, 4, 7};
        hface_t f4s = {3, 4, 7, 6, 1, 2};
        hfaces_t res1 = {{4, 5, 0, 1, 6, 7}};
        hfaces_t res2 = {{1, 0, 5, 4, 7, 6}};

        REQUIRE(res1 == reduce_faces({f3, f4}));
        REQUIRE(res1 == reduce_faces({f3s, f4}));
        REQUIRE(res1 == reduce_faces({f3s, f4s}));
        REQUIRE(res1 == reduce_faces({f3, f4s}));

        std::reverse(f3.begin(), f3.end());
        std::reverse(f4.begin(), f4.end());

        REQUIRE(res2 == reduce_faces({f3, f4}));
        REQUIRE(res1 == reduce_faces({f3s, f4}));
        REQUIRE(res1 == reduce_faces({f3s, f4s}));
        REQUIRE(res2 == reduce_faces({f3, f4s}));
    }
}
