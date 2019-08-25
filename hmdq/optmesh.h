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

#pragma once

#include "xtdef.h"

//  functions
//------------------------------------------------------------------------------
//  Test if vertex `v` is in `verts` vertices. Return index or -1.
long long v_in_verts(const hvector_t& v, const std::vector<hvector_t>& verts);
//
//  Remove duplicate vertices in `verts` and rearrange `faces`.
std::pair<harray2d_t, hfaces_t> reduce_verts(const harray2d_t& verts,
                                             const hfaces_t& faces);

//  Return edges in the face.
hedgelist_t face2edges(const hface_t& face);

//  Sort edges orientation, so the first vertex has lower index.
hedgelist_t sort_edges(const hedgelist_t& edges);

//  Return shared edges.
hedgelist_t shared_edges(const hedgelist_t& edges1, const hedgelist_t& edges2);

//  Reverse the orientation of the edges in the face.
hedgelist_t reverse_edges(const hedgelist_t& edges);

//  Build face from edges.
hface_t build_face(const hedgelist_t& edges1, const hedgelist_t& edges2);

//  Return true if the edges match regardless the direction.
inline bool match_edges(const hedge_t& e1, const hedge_t& e2)
{
    return (e1 == e2 || (e1.first == e2.second && e1.second == e2.first));
}

//  Remove continous chain of edges (sorted) from `edges` and return what
//  remains again continous.
hedgelist_t remove_chain(const hedgelist_t& chain, const hedgelist_t& edges);

//  Check if the edges are chained. If they are, shift the sequence so
//  it starts with the first edge in the chain and ends with the last one.
//  Otherwise return [].
hedgelist_t check_chained(const hedgelist_t& edges);

//  Build a new face from two edge lists and the shared chain.
hface_t merge_edges(const hedgelist_t& edges1, const hedgelist_t& edges2,
                    const hedgelist_t& chain);

// Reduce faces by removing duplicate edges.
hfaces_t reduce_faces(const hfaces_t& faces);
