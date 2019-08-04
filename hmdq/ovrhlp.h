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

#pragma once

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include "hmdview.h"
#include "xtdef.h"

#include "fifo_map_fix.h"

//  typedefs
//------------------------------------------------------------------------------
typedef std::vector<std::pair<vr::EVREye, std::string>> heyes_t;

//  globals
//------------------------------------------------------------------------------
constexpr const char* LEYE = "Left";
constexpr const char* REYE = "Right";
extern const heyes_t EYES;

//  typedefs
//------------------------------------------------------------------------------
typedef std::vector<vr::ETrackedDeviceProperty> hproplist_t;
typedef std::vector<std::pair<vr::TrackedDeviceIndex_t, vr::ETrackedDeviceClass>>
    hdevlist_t;

//  functions
//------------------------------------------------------------------------------
//  Convert hidden area mask mesh from OpenVR to numpy array of vertices.
harray2d_t hmesh2np(const vr::HiddenAreaMesh_t& hmesh);

//  Get hidden area mask (HAM) mesh.
std::pair<harray2d_t, hfaces_t> get_ham_mesh(vr::IVRSystem* vrsys, vr::EVREye eye,
                                             vr::EHiddenAreaMeshType hamtype);

//  Return hidden area mask mesh for given eye.
json get_ham_mesh_opt(vr::IVRSystem* vrsys, vr::EVREye eye,
                      vr::EHiddenAreaMeshType hamtype);

//  Get raw projection values (LRBT) for `eye`.
json get_raw_eye(vr::IVRSystem* vrsys, vr::EVREye eye);

//  Enumerate the attached devices.
hdevlist_t enum_devs(vr::IVRSystem* vrsys, const json& api, int verb = 0, int ind = 0,
                     int ts = 0);

//  Parse OpenVR JSON API definition, where jd = json.load("openvr_api.json")
json parse_json_oapi(const json& jd);

//  Return dict of properties for device `did`.
json get_dev_props(vr::IVRSystem* vrsys, vr::TrackedDeviceIndex_t did,
                   vr::ETrackedDeviceClass dclass, int cat, const json& api, bool anon,
                   const hproplist_t& props_to_hash, bool use_pname = false, int verb = 0,
                   int ind = 0, int ts = 0);

//  Return properties for all devices.
json get_all_props(vr::IVRSystem* vrsys, const hdevlist_t& devs, const json& api,
                   bool anon, bool use_pname = false, int verb = 0, int ind = 0,
                   int ts = 0);

//  Enumerate view and projection geometry for both eyes.
json get_eyes_geometry(vr::IVRSystem* vrsys, const heyes_t& eyes, int verb = 0,
                       int ind = 0, int ts = 0);

//  Checks the OpenVR and HMD presence and return IVRSystem.
vr::IVRSystem* get_vrsys(vr::EVRApplicationType app_type, int verb = 0, int ind = 0,
                         int ts = 0);
