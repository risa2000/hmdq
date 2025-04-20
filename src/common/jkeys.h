/******************************************************************************
 * HMDQ Tools - tools for VR headsets and other hardware introspection        *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2020, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#pragma once

#include <string>

//  JSON constants
//------------------------------------------------------------------------------
constexpr const char* ANON_PREFIX = "anon@";
constexpr const char* ERROR_PREFIX = "error@";

//  JSON keys used in data and config files.
//------------------------------------------------------------------------------

//  External (openvr_api.json)
constexpr const char* j_enums = "enums";
constexpr const char* j_enumname = "enumname";
constexpr const char* j_values = "values";
constexpr const char* j_name = "name";
constexpr const char* j_value = "value";

//  Internal (JSON API extract)
constexpr const char* j_name2id = "name2id";
constexpr const char* j_classes = "classes";

//  Config
constexpr const char* j_meta = "meta";
constexpr const char* j_cfg_ver = "cfg_ver";
constexpr const char* j_prog_ver = "prog_ver";
constexpr const char* j_control = "control";
constexpr const char* j_anonymize = "anonymize";
constexpr const char* j_format = "format";
constexpr const char* j_json_indent = "json_indent";
constexpr const char* j_cli_indent = "cli_indent";
constexpr const char* j_verbosity = "verbosity";
constexpr const char* j_silent = "silent";
constexpr const char* j_default = "default";
constexpr const char* j_max = "max";
constexpr const char* j_error = "error";

//  HMDQ data
//  File miscellanous
constexpr const char* j_leye = "Left";
constexpr const char* j_reye = "Right";
constexpr const char* j_checksum = "checksum";

//  Tool misceallanous
constexpr const char* j_misc = "misc";
constexpr const char* j_time = "time";
constexpr const char* j_hmdq_ver = "hmdq_ver";
constexpr const char* j_hmdv_ver = "hmdv_ver";
constexpr const char* j_log_ver = "log_ver";
constexpr const char* j_os_ver = "os_ver";

//  VR subsystem miscellanous
constexpr const char* j_rt_path = "rt_path";
constexpr const char* j_rt_ver = "rt_ver";

//  VR subsystem devices
constexpr const char* j_devices = "devices";
constexpr const char* j_trackers = "trackers";
constexpr const char* j_ctrl_types = "ctrl_types";

//  VR subsystem properties
constexpr const char* j_properties = "properties";
constexpr const char* j_hmd = "HMD";

//  Headset geometry
constexpr const char* j_geometry = "geometry";
constexpr const char* j_rec_rts = "rec_rts";

//  Eye FOV
constexpr const char* j_raw_eye = "raw_eye";
constexpr const char* j_tan_left = "tan_left";
constexpr const char* j_tan_right = "tan_right";
constexpr const char* j_tan_bottom = "tan_bottom";
constexpr const char* j_tan_top = "tan_top";
constexpr const char* j_aspect = "aspect";

//  Eye to head transformation matrix (OpenVR)
constexpr const char* j_eye2head = "eye2head";

//  Headset camery view geometry
constexpr const char* j_view_geom = "view_geom";
constexpr const char* j_left_rot = "left_rot";
constexpr const char* j_right_rot = "right_rot";
constexpr const char* j_ipd = "ipd";

//  Eye FOV geometry and characteristics
constexpr const char* j_fov_eye = "fov_eye";
constexpr const char* j_fov_head = "fov_head";
constexpr const char* j_fov_pts = "fov_pts";
constexpr const char* j_deg_left = "deg_left";
constexpr const char* j_deg_right = "deg_right";
constexpr const char* j_deg_bottom = "deg_bottom";
constexpr const char* j_deg_top = "deg_top";
constexpr const char* j_deg_hor = "deg_hor";
constexpr const char* j_deg_ver = "deg_ver";

constexpr const char* j_fov_tot = "fov_tot";
constexpr const char* j_fov_hor = "fov_hor";
constexpr const char* j_fov_ver = "fov_ver";
constexpr const char* j_fov_diag = "fov_diag";
constexpr const char* j_overlap = "overlap";

//  Hidden area mesh data
constexpr const char* j_ham_mesh = "ham_mesh";
constexpr const char* j_ham_area = "ham_area";
constexpr const char* j_verts_raw = "verts_raw";
constexpr const char* j_verts_opt = "verts_opt";
constexpr const char* j_faces_opt = "faces_opt";
constexpr const char* j_tris_opt = "tris_opt"; // not used

//  OpenVR specifics
constexpr const char* j_openvr = "openvr";
constexpr const char* j_app_type = "app_type";

//  Oculus specifics
constexpr const char* j_oculus = "oculus";
constexpr const char* j_init_flags = "init_flags";
constexpr const char* j_default_fov = "default_fov";
constexpr const char* j_max_fov = "max_fov";
constexpr const char* j_distorted_viewport = "distorted_viewport";
constexpr const char* j_render_desc = "render_desc";
constexpr const char* j_pixels_per_tan = "pixels_per_tan";
constexpr const char* j_hmd2eye_pose = "hmd2eye_pose";
constexpr const char* j_orientation = "orientation";
constexpr const char* j_position = "position";
constexpr const char* j_faces_raw = "faces_raw";

//  Print friendly names for keys
//------------------------------------------------------------------------------
const std::string& get_jkey_pretty(const std::string& jkey);
