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

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <filesystem>
#include <tuple>

#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include "hmddef.h"
#include "xtdef.h"

#include "fifo_map_fix.h"

//  functions which do not need OpenVR initialized
//------------------------------------------------------------------------------
//  If a HMD is not present abort (does not need IVRSystem).
void is_hmd_present();

//  Return OpenVR runtime path.
std::string get_vr_runtime_path();

//  Return the version of the OpenVR API used in the build.
std::tuple<uint32_t, uint32_t, uint32_t> get_vr_sdk_ver();

//  functions (OpenVR init)
//------------------------------------------------------------------------------
//  Initialize OpenVR subsystem and return IVRSystem interace.
std::tuple<vr::IVRSystem*, vr::EVRInitError> init_vrsys(vr::EVRApplicationType app_type);

//  functions (miscellanous)
//------------------------------------------------------------------------------
//  Return OpenVR version from the runtime.
const char* get_vr_runtime_ver(vr::IVRSystem* vrsys);

//  functions (devices and properties)
//------------------------------------------------------------------------------
//  Enumerate the attached devices.
hdevlist_t enum_devs(vr::IVRSystem* vrsys);

//  Return properties for all devices.
json get_all_props(vr::IVRSystem* vrsys, const hdevlist_t& devs, const json& api);

//  higher level processing functions
//------------------------------------------------------------------------------
//  Return some info about OpenVR.
json get_openvr(vr::IVRSystem* vrsys, const json& api);

//  functions (geometry)
//------------------------------------------------------------------------------
//  Get hidden area mask (HAM) mesh.
//  Return hidden area mask mesh for given eye.
json get_ham_mesh(vr::IVRSystem* vrsys, vr::EVREye eye, vr::EHiddenAreaMeshType hamtype);

//  Get raw projection values (LRBT) for `eye`.
json get_raw_eye(vr::IVRSystem* vrsys, vr::EVREye eye);

//  Get eye to head transform matrix.
json get_eye2head(vr::IVRSystem* vrsys, vr::EVREye eye);

//  Enumerate view and projection geometry for both eyes.
json get_geometry(vr::IVRSystem* vrsys);
