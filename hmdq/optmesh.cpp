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
#include <algorithm>
#include <list>

#include <xtensor/xview.hpp>

#include "geom.h"
#include "optmesh.h"
#include "utils.h"
#include "xtdef.h"

//  functions
//------------------------------------------------------------------------------
//  Test if vertex `v` is in `verts` vertices. Return index or -1.
long long v_in_verts(const hvector_t& v, const hveclist_t& verts)
{
    for (size_t i = 0, e = verts.size(); i < e; ++i) {
        if (v == verts[i]) {
            return i;
        }
    }
    return -1;
}

//  Remove duplicate vertices in `verts` and rearrange `faces`.
std::pair<harray2d_t, hfaces_t> reduce_verts(const harray2d_t& verts,
                                             const hfaces_t& faces)
{
    hveclist_t r_verts;
    hfaces_t r_faces;
    for (const auto f : faces) {
        hface_t new_f;
        for (auto v_i : f) {
            auto i = v_in_verts(xt::view(verts, v_i), r_verts);
            if (i >= 0) {
                new_f.push_back(i);
            }
            else {
                r_verts.push_back(xt::view(verts, v_i));
                new_f.push_back(r_verts.size() - 1);
            }
        }
        r_faces.push_back(new_f);
    }
    // build the new verts
    return std::make_pair(build_array(r_verts), r_faces);
}

//  Return edges in the face.
hedgelist_t face2edges(const hface_t& face)
{
    hedgelist_t res;
    for (size_t i = 0, e = face.size(); i < e; ++i) {
        res.push_back({face[i], face[(i + 1) % e]});
    }
    return res;
}

//  Sort edges orientation, so the first vertex has lower index.
hedgelist_t sort_edges(const hedgelist_t& edges)
{
    hedgelist_t res;
    for (const auto e : edges) {
        if (e.first <= e.second) {
            res.push_back(e);
        }
        else {
            res.push_back({e.second, e.first});
        }
    }
    return res;
}

//  Return shared edges.
hedgelist_t shared_edges(const hedgelist_t& edges1, const hedgelist_t& edges2)
{
    // naive implementaion of an intersection function
    hedgelist_t te1 = sort_edges(edges1);
    hedgelist_t te2 = sort_edges(edges2);
    std::sort(te1.begin(), te1.end());
    std::sort(te2.begin(), te2.end());
    auto f1 = te1.begin();
    auto e1 = te1.end();
    auto f2 = te2.begin();
    auto e2 = te2.end();
    hedgelist_t res;
    while (f1 != e1 && f2 != e2) {
        if (*f1 < *f2) {
            ++f1;
        }
        else if (*f1 > *f2) {
            ++f2;
        }
        else {
            res.push_back(*f1);
            ++f1;
            ++f2;
        }
    }
    return res;
}

//  Reverse the orientation of the edges in the face.
hedgelist_t reverse_edges(const hedgelist_t& edges)
{
    hedgelist_t res;
    for (auto i = edges.rbegin(), e = edges.rend(); i != e; ++i) {
        res.push_back({i->second, i->first});
    }
    return res;
}

//  Build face from edges.
hface_t build_face(const hedgelist_t& edges1, const hedgelist_t& edges2)
{
    hedgelist_t te2;
    hface_t res;

    if (edges1[edges1.size() - 1].second != edges2[0].first) {
        te2 = reverse_edges(edges2);
    }
    else {
        te2 = edges2;
    }

    for (const auto e : edges1) {
        res.push_back(e.first);
    }

    for (const auto e : te2) {
        res.push_back(e.first);
    }
    return res;
}
//  Remove continous chain of edges (sorted) from `edges` and return what
//  remains again continous.
hedgelist_t remove_chain(const hedgelist_t& chain, const hedgelist_t& edges)
{
    long long se = static_cast<long long>(edges.size());
    long long pivot = 0;
    long long count = 0;
    for (long long i = 0; i < se; ++i) {
        if (match_edges(chain[0], edges[i])) {
            // found pivot, now found direction
            if (chain.size() == 1) {
                // if chain is only one edge long match right away
                pivot = i;
                count = chain.size();
                break;
            }
            else if (match_edges(chain[1], edges[(i + 1) % se])) {
                // forward direction
                pivot = i;
                count = chain.size();
                break;
            }
            else if (match_edges(chain[1], edges[mod_pos(i - 1, se)])) {
                // backward direction
                pivot = i;
                count = -static_cast<long long>(chain.size());
                break;
            }
        }
    }

    if (count < 0) {
        if (pivot + count < 0) {
            return hedgelist_t(edges.begin() + pivot + 1,
                               edges.begin() + mod_pos(pivot + count + 1, se));
        }
        else {
            hedgelist_t res(edges.begin() + pivot + 1, edges.end());
            res.insert(res.end(), edges.begin(), edges.begin() + pivot + count + 1);
            return res;
        }
    }
    else if (count > 0) {
        if (pivot + count >= se) {
            return hedgelist_t(edges.begin() + (pivot + count) % se,
                               edges.begin() + pivot);
        }
        else {
            hedgelist_t res(edges.begin() + pivot + count, edges.end());
            res.insert(res.end(), edges.begin(), edges.begin() + pivot);
            return res;
        }
    }
    else {
        // technically this should match one way or the other
        HMDQ_ASSERT(count != 0);
        return hedgelist_t(); // dummy return to satisfy the compiler
    }
}

//  Build a new face from two edge lists and the shared chain.
hface_t merge_edges(const hedgelist_t& edges1, const hedgelist_t& edges2,
                    const hedgelist_t& chain)
{
    auto tes1 = remove_chain(chain, edges1);
    auto tes2 = remove_chain(chain, edges2);
    return build_face(tes1, tes2);
}

//  Check if the edges are chained. If they are, shift the sequence so
//  it starts with the first edge in the chain and ends with the last one.
//  Otherwise return [].
hedgelist_t check_chained(const hedgelist_t& edges)
{
    auto se = edges.size();
    if (edges.size() <= 1) {
        return edges;
    }

    int chained = 0;
    size_t split = 0;
    for (size_t i = 0; i < se; ++i) {
        if (edges[i].first == edges[(i + 1) % se].first) {
            chained += 1;
        }
        else if (edges[i].first == edges[(i + 1) % se].second) {
            chained += 1;
        }
        else if (edges[i].second == edges[(i + 1) % se].first) {
            chained += 1;
        }
        else if (edges[i].first == edges[(i + 1) % se].second) {
            chained += 1;
        }
        else {
            split = i;
        }
    }

    // if there were more than one disconnect return []
    if (chained < se - 1) {
        return hedgelist_t();
    }
    // if only two edges are chained no need for the shift
    if (se == 2) {
        return edges;
    }
    // "shift" the edges so the split is at the beginning
    hedgelist_t res(edges.begin() + split + 1, edges.end());
    res.insert(res.end(), edges.begin(), edges.begin() + split + 1);
    return res;
}

// Reduce faces by removing duplicate edges.
hfaces_t reduce_faces(const hfaces_t& faces)
{
    // result
    hfaces_t nfaces;
    // faces to process
    std::list<hface_t> tfaces(faces.begin(), faces.end());

    while (tfaces.size() >= 2) {
        // run the loop until there are at least two faces to check
        hface_t face = tfaces.front();
        tfaces.pop_front();
        bool found = true;
        while (found) {
            hfaces_t checked;
            found = false;
            while (tfaces.size()) {
                hface_t f = tfaces.front();
                tfaces.pop_front();
                hedgelist_t edges1 = face2edges(face);
                hedgelist_t edges2 = face2edges(f);
                hedgelist_t shared = shared_edges(edges1, edges2);
                if (shared.size()) {
                    auto chain = check_chained(shared);
                    // if shared egdes are not chained we are probably not looking at the
                    // triangle mesh
                    HMDQ_ASSERT(chain.size() != 0);
                    face = merge_edges(edges1, edges2, chain);
                    found = true;
                }
                else {
                    checked.push_back(f);
                }
            }
            tfaces.assign(checked.begin(), checked.end());
        }
        nfaces.push_back(face);
    }
    nfaces.insert(nfaces.end(), tfaces.begin(), tfaces.end());
    return nfaces;
}
