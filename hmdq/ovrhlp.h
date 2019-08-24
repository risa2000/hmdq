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

#include "xtdef.h"

#include "fifo_map_fix.h"

//  typedefs
//------------------------------------------------------------------------------
typedef std::vector<vr::ETrackedDeviceProperty> hproplist_t;
typedef std::vector<std::pair<vr::TrackedDeviceIndex_t, vr::ETrackedDeviceClass>>
    hdevlist_t;

//  functions (miscellanous)
//------------------------------------------------------------------------------
//  Return OpenVR version from the runtime.
const char* get_openvr_ver(vr::IVRSystem* vrsys);

//  functions (devices and properties)
//------------------------------------------------------------------------------
//  Enumerate the attached devices.
hdevlist_t enum_devs(vr::IVRSystem* vrsys);

//  Return properties for all devices.
json get_all_props(vr::IVRSystem* vrsys, const hdevlist_t& devs, const json& api,
                   bool anon);

//  functions (geometry)
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

//  Enumerate view and projection geometry for both eyes.
json get_geometry(vr::IVRSystem* vrsys);

//  functions (OpenVR init)
//------------------------------------------------------------------------------
//  If a HMD is not present abort (does not need IVRSystem).
void is_hmd_present();

//  Return OpenVR runtime path.
std::string get_vr_runtime_path();

//  Initialize OpenVR subsystem and return IVRSystem interace.
vr::IVRSystem* init_vrsys(vr::EVRApplicationType app_type);
