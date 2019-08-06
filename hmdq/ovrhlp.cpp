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

#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include <xtensor/xadapt.hpp>
#include <xtensor/xjson.hpp>
#include <xtensor/xview.hpp>

#include <fmt/format.h>

#include "config.h"
#include "fmthlp.h"
#include "geom.h"
#include "hmdview.h"
#include "jtools.h"
#include "optmesh.h"
#include "ovrhlp.h"
#include "prtdata.h"
#include "xtdef.h"

#include "fifo_map_fix.h"

//  globals
//------------------------------------------------------------------------------
const heyes_t EYES = {{vr::Eye_Left, LEYE}, {vr::Eye_Right, REYE}};

//  locals
//------------------------------------------------------------------------------
static constexpr size_t BUFFSIZE = 256;

static const int PROP_CAT_COMMON = 1;
static const int PROP_CAT_HMD = 2;
static const int PROP_CAT_CONTROLLER = 3;
static const int PROP_CAT_TRACKEDREF = 4;

//  properties to hash for PROPS_TO_HASH to "seed" (differentiate) same S/N from
//  different manufacturers (in this order)
static const hproplist_t PROPS_TO_SEED
    = {vr::Prop_ManufacturerName_String, vr::Prop_ModelNumber_String};

//  OpenVR API loader
//------------------------------------------------------------------------------
//  Parse OpenVR JSON API definition, where jd = json.load("openvr_api.json")
json parse_json_oapi(const json& jd)
{
    json tdprops;
    json tdcls;
    for (auto e : jd["enums"]) {
        if (e["enumname"].get<std::string>() == "vr::ETrackedDeviceProperty") {
            for (auto v : e["values"]) {
                auto name = v["name"].get<std::string>();
                // val type is actually vr::ETrackedDeviceProperty
                auto val = std::stoi(v["value"].get<std::string>());
                auto cat = static_cast<int>(val) / 1000;
                tdprops[std::to_string(cat)][std::to_string(val)] = name;
                tdprops["name2id"][name] = val;
            }
        }
        else if (e["enumname"].get<std::string>() == "vr::ETrackedDeviceClass") {
            for (auto v : e["values"]) {
                auto name = v["name"].get<std::string>();
                // val type is actually vr::ETrackedDeviceClass
                auto val = std::stoi(v["value"].get<std::string>());
                auto fs = name.find('_');
                if (fs != std::string::npos) {
                    name = name.substr(fs + 1, std::string::npos);
                }
                tdcls[std::to_string(val)] = name;
            }
        }
    }
    return json({{"classes", tdcls}, {"props", tdprops}});
}

//  functions (devices and properties)
//------------------------------------------------------------------------------
//  Enumerate the attached devices.
hdevlist_t enum_devs(vr::IVRSystem* vrsys)
{
    hdevlist_t res;
    for (vr::TrackedDeviceIndex_t dev_id = 0; dev_id < vr::k_unMaxTrackedDeviceCount;
         ++dev_id) {
        auto dev_class = vrsys->GetTrackedDeviceClass(dev_id);
        if (dev_class != vr::TrackedDeviceClass_Invalid) {
            res.push_back(std::make_pair(dev_id, dev_class));
        }
    }
    return res;
}

//  Resolve property tag enum from the type name.
vr::PropertyTypeTag_t get_ptag_from_ptype(const std::string& ptype)
{
    if (ptype == "Float") {
        return vr::k_unFloatPropertyTag;
    }
    else if (ptype == "Int32") {
        return vr::k_unInt32PropertyTag;
    }
    else if (ptype == "Uint64") {
        return vr::k_unUint64PropertyTag;
    }
    else if (ptype == "Bool") {
        return vr::k_unBoolPropertyTag;
    }
    else if (ptype == "String") {
        return vr::k_unBoolPropertyTag;
    }
    else if (ptype == "Matrix34") {
        return vr::k_unHmdMatrix34PropertyTag;
    }
    else if (ptype == "Matrix44") {
        return vr::k_unHmdMatrix44PropertyTag;
    }
    else if (ptype == "Vector2") {
        return vr::k_unHmdVector2PropertyTag;
    }
    else if (ptype == "Vector3") {
        return vr::k_unHmdVector3PropertyTag;
    }
    else if (ptype == "Vector4") {
        return vr::k_unHmdVector4PropertyTag;
    }
    else if (ptype == "Quad") {
        return vr::k_unHmdQuadPropertyTag;
    }
    else {
        return vr::k_unInvalidPropertyTag;
    }
}

//  Check the returned value and print out the error message if detected.
inline bool check_tp_result(vr::IVRSystem* vrsys, json& jd, const std::string& pname,
                            vr::ETrackedPropertyError res)
{
    if (res == vr::TrackedProp_Success) {
        return true;
    }
    auto msg = fmt::format("{:s}", vrsys->GetPropErrorNameFromEnum(res));
    jd[pname][ERROR_PREFIX] = msg;
    return false;
}

//  Set TrackedPropertyValue (vector value) in the JSON dict.
template<typename T>
void set_tp_val_1d_array(json& jd, const std::string& pname,
                         const std::vector<unsigned char>& buffer, size_t buffsize)
{
    // we got float array in buffer of buffsize / sizeof(float)
    auto ptype = reinterpret_cast<const T*>(&buffer[0]);
    auto size = buffsize / sizeof(T);
    std::vector<std::size_t> shape = {size};
    xt::xtensor<T, 1> atype = xt::adapt(ptype, size, xt::no_ownership(), shape);
    jd[pname] = atype;
}

//  Set TrackedPropertyValue (matrix array value) in the JSON dict.
template<typename M>
void set_tp_val_mat_array(json& jd, const std::string& pname,
                          const std::vector<unsigned char>& buffer, size_t buffsize)
{
    // we got float array in buffer of buffsize / sizeof(float)
    auto pfloat = reinterpret_cast<const float*>(&buffer[0]);
    auto size = buffsize / sizeof(float);
    auto constexpr nrows = sizeof(M::m) / sizeof(M::m[0]);
    auto constexpr ncols = sizeof(M::m[0]) / sizeof(float);
    std::vector<std::size_t> shape = {buffsize / sizeof(M::m), nrows, ncols};
    harray_t amats = xt::adapt(pfloat, size, xt::no_ownership(), shape);
    jd[pname] = amats;
}

//  Set TrackedPropertyValue (vector array value) in the JSON dict.
template<typename V, typename I = float>
void set_tp_val_vec_array(json& jd, const std::string& pname,
                          const std::vector<unsigned char>& buffer, size_t buffsize)
{
    // we got I type array in buffer of buffsize / sizeof(I)
    auto pitem = reinterpret_cast<const I*>(&buffer[0]);
    auto size = buffsize / sizeof(I);
    auto constexpr vsize = sizeof(V::v) / sizeof(I);
    std::vector<std::size_t> shape = {buffsize / sizeof(V::v), vsize};
    harray_t avecs = xt::adapt(pitem, size, xt::no_ownership(), shape);
    jd[pname] = avecs;
}

//  Universal routine to get any "Array" property into the JSON dict.
void get_array_type(vr::IVRSystem* vrsys, json& jd, vr::TrackedDeviceIndex_t did,
                    vr::ETrackedDeviceProperty pid, const std::string& pname)
{
    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    // abuse vector as a return buffer for an Array property
    std::vector<unsigned char> buffer(BUFFSIZE);

    // find the name of the type (before "_Array" suffix)
    auto sarray = pname.rfind('_');
    auto stype = pname.rfind('_', sarray - 1) + 1;
    auto ptype = pname.substr(stype, sarray - stype);
    vr::PropertyTypeTag_t ptag = get_ptag_from_ptype(ptype);

    if (ptag == vr::k_unInvalidPropertyTag) {
        auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype);
        jd[pname][ERROR_PREFIX] = msg;
        return;
    }

    size_t buffsize = vrsys->GetArrayTrackedDeviceProperty(
        did, pid, ptag, &buffer[0], static_cast<uint32_t>(buffer.size()), &error);
    if (error == vr::TrackedProp_BufferTooSmall) {
        // resize buffer
        buffer.resize(buffsize);
        buffsize = vrsys->GetArrayTrackedDeviceProperty(
            did, pid, ptag, &buffer[0], static_cast<uint32_t>(buffer.size()), &error);
    }
    if (!check_tp_result(vrsys, jd, pname, error)) {
        return;
    }

    if (ptag == vr::k_unFloatPropertyTag) {
        set_tp_val_1d_array<float>(jd, pname, buffer, buffsize);
    }
    else if (ptag == vr::k_unInt32PropertyTag) {
        set_tp_val_1d_array<int32_t>(jd, pname, buffer, buffsize);
    }
    else if (ptag == vr::k_unUint64PropertyTag) {
        set_tp_val_1d_array<uint64_t>(jd, pname, buffer, buffsize);
    }
    else if (ptag == vr::k_unBoolPropertyTag) {
        set_tp_val_1d_array<bool>(jd, pname, buffer, buffsize);
    }
    else if (ptag == vr::k_unHmdMatrix34PropertyTag) {
        set_tp_val_mat_array<vr::HmdMatrix34_t>(jd, pname, buffer, buffsize);
    }
    else if (ptag == vr::k_unHmdMatrix44PropertyTag) {
        set_tp_val_mat_array<vr::HmdMatrix44_t>(jd, pname, buffer, buffsize);
    }
    else if (ptag == vr::k_unHmdVector2PropertyTag) {
        set_tp_val_vec_array<vr::HmdVector2_t>(jd, pname, buffer, buffsize);
    }
    else if (ptag == vr::k_unHmdVector3PropertyTag) {
        set_tp_val_vec_array<vr::HmdVector3_t>(jd, pname, buffer, buffsize);
    }
    else if (ptag == vr::k_unHmdVector4PropertyTag) {
        set_tp_val_vec_array<vr::HmdVector4_t>(jd, pname, buffer, buffsize);
    }
    else {
        auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype);
        jd[pname][ERROR_PREFIX] = msg;
    }
}

//  Get string tracked property (this is a helper to isolate the buffer handling).
bool get_str_tracked_prop(vr::IVRSystem* vrsys, json& jd, vr::TrackedDeviceIndex_t did,
                          vr::ETrackedDeviceProperty pid, const std::string& pname,
                          std::vector<char>& buffer)
{
    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    size_t buffsize = vrsys->GetStringTrackedDeviceProperty(
        did, pid, &buffer[0], static_cast<uint32_t>(buffer.size()), &error);
    if (error == vr::TrackedProp_BufferTooSmall) {
        // resize buffer
        buffer.resize(buffsize);
        buffsize = vrsys->GetStringTrackedDeviceProperty(
            did, pid, &buffer[0], static_cast<uint32_t>(buffer.size()), &error);
    }
    return check_tp_result(vrsys, jd, pname, error);
}

//  Get hashed value of the string, pre-seeded with PROPS_TO_SEED values.
void get_str_hashed(vr::IVRSystem* vrsys, vr::TrackedDeviceIndex_t did,
                    const std::string& pname, std::vector<char>& buffer)
{
    std::vector<char> tempbuff(BUFFSIZE);
    std::vector<char> msgbuff;
    // first hash the "seed"
    json empty;
    for (auto pid : PROPS_TO_SEED) {
        if (get_str_tracked_prop(vrsys, empty, did, pid, pname, tempbuff)) {
            std::copy(tempbuff.begin(), tempbuff.begin() + std::strlen(&tempbuff[0]),
                      std::back_inserter(msgbuff));
        }
    }
    // finally add the actual S/N
    std::copy(buffer.begin(), buffer.begin() + std::strlen(&buffer[0]),
              std::back_inserter(msgbuff));
    // and the terminating '\0'
    msgbuff.push_back('\0');
    // anonymize
    anonymize(buffer, msgbuff);
}

//  Return dict of properties for device `did`.
json get_dev_props(vr::IVRSystem* vrsys, vr::TrackedDeviceIndex_t did,
                   vr::ETrackedDeviceClass dclass, int cat, const json& api, bool anon)
{
    const hproplist_t props_to_hash = g_cfg["control"]["anon_props"].get<hproplist_t>();
    const auto scat = std::to_string(cat);

    json res;

    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    // abuse vector as a return buffer for the String property
    std::vector<char> buffer(BUFFSIZE);

    for (auto [spid, jname] : api["props"][scat].items()) {
        // convert string to the correct type
        auto pid = static_cast<vr::ETrackedDeviceProperty>(std::stoi(spid));
        // property name
        auto pname = jname.get<std::string>();
        // property type (the last part after '_')
        auto ptype = pname.substr(pname.rfind('_') + 1);

        if (ptype == "Bool") {
            auto pval = vrsys->GetBoolTrackedDeviceProperty(did, pid, &error);
            if (!check_tp_result(vrsys, res, pname, error)) {
                continue;
            }
            res[pname] = pval;
        }
        else if (ptype == "String") {
            if (!get_str_tracked_prop(vrsys, res, did, pid, pname, buffer)) {
                continue;
            }
            if (anon
                && (props_to_hash.end()
                    != std::find(props_to_hash.begin(), props_to_hash.end(), pid))) {
                get_str_hashed(vrsys, did, pname, buffer);
            }
            res[pname] = &buffer[0];
        }
        else if (ptype == "Uint64") {
            auto pval = vrsys->GetUint64TrackedDeviceProperty(did, pid, &error);
            if (!check_tp_result(vrsys, res, pname, error)) {
                continue;
            }
            res[pname] = pval;
        }
        else if (ptype == "Int32") {
            auto pval = vrsys->GetInt32TrackedDeviceProperty(did, pid, &error);
            if (!check_tp_result(vrsys, res, pname, error)) {
                continue;
            }
            res[pname] = pval;
        }
        else if (ptype == "Float") {
            auto pval = vrsys->GetFloatTrackedDeviceProperty(did, pid, &error);
            if (!check_tp_result(vrsys, res, pname, error)) {
                continue;
            }
            res[pname] = pval;
        }
        else if (ptype == "Matrix34") {
            auto pval = vrsys->GetMatrix34TrackedDeviceProperty(did, pid, &error);
            if (!check_tp_result(vrsys, res, pname, error)) {
                continue;
            }
            std::vector<std::size_t> shape = {3, 4};
            auto mat34 = xt::adapt(&pval.m[0][0], shape);
            res[pname] = mat34;
        }
        else if (ptype == "Array") {
            get_array_type(vrsys, res, did, pid, pname);
        }
        else {
            auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype);
            res[pname][ERROR_PREFIX] = msg;
        }
    }
    return res;
}

//  Return properties for all devices.
json get_all_props(vr::IVRSystem* vrsys, const hdevlist_t& devs, const json& api,
                   bool anon)
{
    json pvals;

    for (auto [did, dclass] : devs) {
        auto sdid = std::to_string(did);
        pvals[sdid] = get_dev_props(vrsys, did, dclass, PROP_CAT_COMMON, api, anon);
        if (dclass == vr::TrackedDeviceClass_HMD) {
            pvals[sdid].update(
                get_dev_props(vrsys, did, dclass, PROP_CAT_HMD, api, anon));
        }
        else if (dclass == vr::TrackedDeviceClass_Controller) {
            pvals[sdid].update(
                get_dev_props(vrsys, did, dclass, PROP_CAT_CONTROLLER, api, anon));
        }
        else if (dclass == vr::TrackedDeviceClass_TrackingReference) {
            pvals[sdid].update(
                get_dev_props(vrsys, did, dclass, PROP_CAT_TRACKEDREF, api, anon));
        }
    }
    return pvals;
}

//  functions (geometry)
//------------------------------------------------------------------------------
//  Convert hidden area mask mesh from OpenVR to numpy array of vertices.
harray2d_t hmesh2np(const vr::HiddenAreaMesh_t& hmesh)
{
    // vectors are 2D points in UV space
    constexpr auto dim = 2;
    auto size = hmesh.unTriangleCount * 3 * dim;
    std::vector<std::size_t> shape
        = {static_cast<size_t>(hmesh.unTriangleCount) * 3, dim};
    harray2d_t lverts
        = xt::adapt(&hmesh.pVertexData[0].v[0], size, xt::no_ownership(), shape);
    return lverts;
}

//  Get hidden area mask (HAM) mesh.
std::pair<harray2d_t, hfaces_t> get_ham_mesh(vr::IVRSystem* vrsys, vr::EVREye eye,
                                             vr::EHiddenAreaMeshType hamtype)
{
    auto hmesh = vrsys->GetHiddenAreaMesh(eye, hamtype);
    if (hmesh.unTriangleCount == 0) {
        return std::make_pair(harray2d_t(), hfaces_t());
    }
    auto verts = hmesh2np(hmesh);
    hfaces_t faces;
    // number of vertices must be divisible by 3 as each 3 defined one triangle
    HMDQ_ASSERT(verts.shape(0) % 3 == 0);
    for (size_t i = 0, e = verts.shape(0); i < e; i += 3) {
        faces.push_back(hface_t({i, i + 1, i + 2}));
    }
    return std::make_pair(verts, faces);
}

//  Return hidden area mask mesh for given eye.
json get_ham_mesh_opt(vr::IVRSystem* vrsys, vr::EVREye eye,
                      vr::EHiddenAreaMeshType hamtype)
{
    auto [verts, faces] = get_ham_mesh(vrsys, eye, hamtype);
    if (faces.empty()) {
        return json();
    }
    auto [n_verts, n_faces] = reduce_verts(verts, faces);
    auto n2_faces = reduce_faces(n_faces);
    auto area = area_mesh(verts);
    json mesh;
    mesh["ham_area"] = area;
    mesh["verts_raw"] = verts;
    mesh["verts_opt"] = n_verts;
    mesh["faces_opt"] = n2_faces;
    return mesh;
}
//  Get raw projection values (LRBT) for `eye`.
json get_raw_eye(vr::IVRSystem* vrsys, vr::EVREye eye)
{
    float left, right, bottom, top;
    // NOTE: the API doc has swapped values for top and bottom
    vrsys->GetProjectionRaw(eye, &left, &right, &bottom, &top);
    auto aspect = (right - left) / (top - bottom);
    json res = {{"tan_left", left},
                {"tan_right", right},
                {"tan_bottom", bottom},
                {"tan_top", top},
                {"aspect", aspect}};
    return res;
}

//  Enumerate view and projection geometry for both eyes.
json get_geometry(vr::IVRSystem* vrsys)
{
    // all the data are collected into specific `json`s
    json eye2head;
    json raw_eye;
    json fov_eye;
    json fov_head;
    json ham_mesh;
    json res;

    uint32_t rec_width, rec_height;
    vrsys->GetRecommendedRenderTargetSize(&rec_width, &rec_height);
    std::vector<uint32_t> rec_rts = {rec_width, rec_height};

    for (auto [eye, neye] : EYES) {

        // get HAM mesh (if supported by the headset, otherwise 'null')
        ham_mesh[neye] = get_ham_mesh_opt(vrsys, eye, vr::k_eHiddenAreaMesh_Standard);

        // get eye to head transformation matrix
        auto oe2h = vrsys->GetEyeToHeadTransform(eye);
        std::vector<std::size_t> shape = {3, 4};
        auto e2h = xt::adapt(&oe2h.m[0][0], shape);
        eye2head[neye] = e2h;

        // get raw eye values (direct from OpenVR)
        raw_eye[neye] = get_raw_eye(vrsys, eye);

        // build eye FOV points only if the eye FOV is rotated
        if (xt::view(e2h, xt::all(), xt::range(0, 3)) != xt::eye<double>(3, 0)) {
            fov_eye[neye] = get_fov(raw_eye[neye], ham_mesh[neye]);
        }

        // build head FOV points (they are eye FOV points if the views are parallel)
        harray2d_t rot = xt::view(e2h, xt::all(), xt::range(0, 3));
        fov_head[neye] = get_fov(raw_eye[neye], ham_mesh[neye], &rot);
    }

    // calculate total FOVs and the overlap
    auto fov_tot = get_total_fov(fov_head);

    // calculate view rotation and the IPD
    auto view_geom = get_view_geom(eye2head);

    res["rec_rts"] = rec_rts;
    res["raw_eye"] = raw_eye;
    res["eye2head"] = eye2head;
    res["view_geom"] = view_geom;
    res["fov_eye"] = fov_eye;
    res["fov_head"] = fov_head;
    res["fov_tot"] = fov_tot;
    res["ham_mesh"] = ham_mesh;

    return res;
}

//  Checks the OpenVR and HMD presence and return IVRSystem.
vr::IVRSystem* get_vrsys(vr::EVRApplicationType app_type, int verb, int ind, int ts)
{
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    const auto sf = ind * ts;

    if (vr::VR_IsRuntimeInstalled()) {
        constexpr size_t cbuffsize = 256;
        std::vector<char> buffer(cbuffsize);
        uint32_t buffsize = 0;
        bool res = vr::VR_GetRuntimePath(&buffer[0], static_cast<uint32_t>(buffer.size()),
                                         &buffsize);
        if (!res) {
            buffer.resize(buffsize);
            res = vr::VR_GetRuntimePath(&buffer[0], static_cast<uint32_t>(buffer.size()),
                                        &buffsize);
        }
        if (res) {
            if (verb >= vdef) {
                iprint(sf, "OpenVR runtime path: {:s}\n", &buffer[0]);
            }
        }
        else {
            iprint(stderr, sf, "Error: Cannot get the runtime path\n");
            return nullptr;
        }
    }
    else {
        iprint(stderr, sf, "Error: No OpenVR runtime found\n");
        return nullptr;
    }

    if (!vr::VR_IsHmdPresent()) {
        iprint(sf, "Error: No HMD found\n");
        return nullptr;
    }

    vr::EVRInitError eError = vr::VRInitError_None;
    vr::IVRSystem* vrsys = vr::VR_Init(&eError, app_type);

    if (eError != vr::VRInitError_None) {
        vrsys = NULL;
        iprint(sf, "Error: {:s}\n", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        return nullptr;
    }
    return vrsys;
}
