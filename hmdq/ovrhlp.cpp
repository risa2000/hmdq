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
#include <iomanip>

#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include <xtensor/xadapt.hpp>
#include <xtensor/xarray.hpp>
#include <xtensor/xeval.hpp>
#include <xtensor/xio.hpp>
#include <xtensor/xjson.hpp>
#include <xtensor/xview.hpp>

#include "config.h"
#include "geom.h"
#include "optmesh.h"
#include "ovrhlp.h"
#include "utils.h"
#include "xtdef.h"

#include "fifo_map_fix.h"

//  globals
//------------------------------------------------------------------------------
const heyes_t EYES = {{vr::Eye_Left, LEYE}, {vr::Eye_Right, REYE}};

//  locals
//------------------------------------------------------------------------------
static const int PROP_CAT_COMMON = 1;
static const int PROP_CAT_HMD = 2;
static const int PROP_CAT_CONTROLLER = 3;
static const int PROP_CAT_TRACKEDREF = 4;

static constexpr const char* DEG = " deg";
static constexpr const char* MM = " mm";
static constexpr const char* PRCT = " %";

//  functions
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

//  Enumerate the attached devices.
hdevlist_t enum_devs(vr::IVRSystem* vrsys, const json& api, int verb, int ind, int ts)
{
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    const std::string sf1(ts * (ind + 1), ' ');

    // space fill at the beginning of the line
    if (verb >= vdef) {
        std::cout << std::string(ind * ts, ' ') << "Device enumeration:\n";
    }
    hdevlist_t res;
    for (vr::TrackedDeviceIndex_t dev_id = 0; dev_id < vr::k_unMaxTrackedDeviceCount;
         ++dev_id) {
        auto dev_class = vrsys->GetTrackedDeviceClass(dev_id);
        if (dev_class != vr::TrackedDeviceClass_Invalid) {
            if (verb >= vdef) {
                std::cout << sf1 << "Found dev: id=" << dev_id << ", class=" << dev_class
                          << ", name="
                          << api["classes"][std::to_string(dev_class)].get<std::string>()
                          << '\n';
            }
            res.push_back(std::make_pair(dev_id, dev_class));
        }
    }
    return res;
}

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

//  Set TrackedDeviceProperty value in the JSON dict.
template<typename T>
inline void set_tp_val(json& j, const std::string& spid, const std::string& pname, T pval,
                       bool use_pname)
{
    if (use_pname) {
        j[pname] = pval;
    }
    else {
        j[spid] = pval;
    }
}

//  Set TrackedDeviceProperty value (which is an xt:xarray) in the JSON dict
inline void set_tp_val_xt(json& j, const std::string& spid, const std::string& pname,
                          const xt::xarray<float>& pval, bool use_pname)
{
    if (use_pname) {
        j[pname] = pval;
    }
    else {
        j[spid] = pval;
    }
}

//  Print the property name to stdout.
inline void prop_head_out(const std::string& spid, const std::string& pname, int ind = 0,
                          int ts = 0)
{
    std::cout << std::string(static_cast<size_t>(ind) * ts, ' ') << std::setw(4) << spid
              << " : " << pname << " = ";
}

//  Check the returned value and print out the error message if detected.
inline bool check_tp_result(vr::IVRSystem* vrsys, vr::ETrackedPropertyError res,
                            const std::string& spid, std::string& pname, int verb = 0,
                            int verr = 0, int ind = 0, int ts = 0)
{
    if (res == vr::TrackedProp_Success) {
        return true;
    }
    if (verb >= verr) {
        prop_head_out(spid, pname, ind, ts);
        std::cout << "[error: " << vrsys->GetPropErrorNameFromEnum(res) << "]\n";
    }
    return false;
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

//  Set TrackedPropertyValue (vector value) in the JSON dict
template<typename T>
void set_tp_val_1d_array(json& j, const std::string& spid, const std::string& pname,
                         const std::vector<unsigned char>& buffer, size_t buffsize,
                         bool use_pname, bool bverb, int ind, int ts)
{
    // we got float array in buffer of buffsize / sizeof(float)
    auto ptype = reinterpret_cast<const T*>(&buffer[0]);
    auto size = buffsize / sizeof(T);
    std::vector<std::size_t> shape = {size};
    xt::xtensor<T, 1> atype = xt::adapt(ptype, size, xt::no_ownership(), shape);

    if (use_pname) {
        j[pname] = atype;
    }
    else {
        j[spid] = atype;
    }

    if (bverb) {
        prop_head_out(spid, pname, ind, ts);
        std::cout << '\n';
        print_array(atype, ind + 1, ts);
    }
}

//  Set TrackedPropertyValue (matrix array value) in the JSON dict
template<typename M>
void set_tp_val_mat_array(json& j, const std::string& spid, const std::string& pname,
                          const std::vector<unsigned char>& buffer, size_t buffsize,
                          bool use_pname, bool bverb, int ind, int ts)
{
    // we got float array in buffer of buffsize / sizeof(float)
    auto pfloat = reinterpret_cast<const float*>(&buffer[0]);
    auto size = buffsize / sizeof(float);
    auto constexpr nrows = sizeof(M::m) / sizeof(M::m[0]);
    auto constexpr ncols = sizeof(M::m[0]) / sizeof(float);
    std::vector<std::size_t> shape = {buffsize / sizeof(M::m), nrows, ncols};
    harray_t amats = xt::adapt(pfloat, size, xt::no_ownership(), shape);

    if (use_pname) {
        j[pname] = amats;
    }
    else {
        j[spid] = amats;
    }
    if (bverb) {
        prop_head_out(spid, pname, ind, ts);
        std::cout << '\n';
        print_array(amats, ind + 1, ts);
    }
}

//  Set TrackedPropertyValue (vector array value) in the JSON dict
template<typename V, typename I = float>
void set_tp_val_vec_array(json& j, const std::string& spid, const std::string& pname,
                          const std::vector<unsigned char>& buffer, size_t buffsize,
                          bool use_pname, bool bverb, int ind, int ts)
{
    // we got I type array in buffer of buffsize / sizeof(I)
    auto pitem = reinterpret_cast<const I*>(&buffer[0]);
    auto size = buffsize / sizeof(I);
    auto constexpr vsize = sizeof(V::v) / sizeof(I);
    std::vector<std::size_t> shape = {buffsize / sizeof(V::v), vsize};
    harray_t avecs = xt::adapt(pitem, size, xt::no_ownership(), shape);

    if (use_pname) {
        j[pname] = avecs;
    }
    else {
        j[spid] = avecs;
    }
    if (bverb) {
        prop_head_out(spid, pname, ind, ts);
        std::cout << '\n';
        print_array(avecs, ind + 1, ts);
    }
}

//  Universal routine to get any "Array" property into the JSON dict
void get_array_type(vr::IVRSystem* vrsys, json& res, vr::TrackedDeviceIndex_t did,
                    vr::ETrackedDeviceProperty pid, const std::string& spid,
                    std::string& pname, int pverb, bool use_pname, int verb, int verr,
                    int ind, int ts)
{
    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    // abuse vector as a return buffer for an Array property
    constexpr size_t BUFFSIZE = 256;
    std::vector<unsigned char> buffer(BUFFSIZE);

    // find the name of the type (before "_Array" suffix)
    auto sarray = pname.rfind('_');
    auto stype = pname.rfind('_', sarray - 1) + 1;
    auto ptype = pname.substr(stype, sarray - stype);
    vr::PropertyTypeTag_t ptag = get_ptag_from_ptype(ptype);

    if (ptag == vr::k_unInvalidPropertyTag) {
        if (verb >= verr) {
            prop_head_out(spid, pname, ind, ts);
            std::cout << "[property type not implemented]\n";
        }
    }

    size_t buffsize = vrsys->GetArrayTrackedDeviceProperty(
        did, pid, ptag, &buffer[0], static_cast<uint32_t>(buffer.size()), &error);
    if (error == vr::TrackedProp_BufferTooSmall) {
        // resize buffer
        buffer.resize(buffsize);
        buffsize = vrsys->GetArrayTrackedDeviceProperty(
            did, pid, ptag, &buffer[0], static_cast<uint32_t>(buffer.size()), &error);
    }
    if (!check_tp_result(vrsys, error, spid, pname, verb, verr, ind, ts)) {
        return;
    }

    if (ptag == vr::k_unFloatPropertyTag) {
        set_tp_val_1d_array<float>(res, spid, pname, buffer, buffsize, use_pname,
                                   verb >= pverb, ind, ts);
    }
    else if (ptag == vr::k_unInt32PropertyTag) {
        set_tp_val_1d_array<int32_t>(res, spid, pname, buffer, buffsize, use_pname,
                                     verb >= pverb, ind, ts);
    }
    else if (ptag == vr::k_unUint64PropertyTag) {
        set_tp_val_1d_array<uint64_t>(res, spid, pname, buffer, buffsize, use_pname,
                                      verb >= pverb, ind, ts);
    }
    else if (ptag == vr::k_unBoolPropertyTag) {
        set_tp_val_1d_array<bool>(res, spid, pname, buffer, buffsize, use_pname,
                                  verb >= pverb, ind, ts);
    }
    else if (ptag == vr::k_unHmdMatrix34PropertyTag) {
        set_tp_val_mat_array<vr::HmdMatrix34_t>(res, spid, pname, buffer, buffsize,
                                                use_pname, verb >= pverb, ind, ts);
    }
    else if (ptag == vr::k_unHmdMatrix44PropertyTag) {
        set_tp_val_mat_array<vr::HmdMatrix44_t>(res, spid, pname, buffer, buffsize,
                                                use_pname, verb >= pverb, ind, ts);
    }
    else if (ptag == vr::k_unHmdVector2PropertyTag) {
        set_tp_val_vec_array<vr::HmdVector2_t>(res, spid, pname, buffer, buffsize,
                                               use_pname, verb >= pverb, ind, ts);
    }
    else if (ptag == vr::k_unHmdVector3PropertyTag) {
        set_tp_val_vec_array<vr::HmdVector3_t>(res, spid, pname, buffer, buffsize,
                                               use_pname, verb >= pverb, ind, ts);
    }
    else if (ptag == vr::k_unHmdVector4PropertyTag) {
        set_tp_val_vec_array<vr::HmdVector4_t>(res, spid, pname, buffer, buffsize,
                                               use_pname, verb >= pverb, ind, ts);
    }
    else {
        if (verb >= verr) {
            prop_head_out(spid, pname, ind, ts);
            std::cout << "[property type not implemented]\n";
        }
    }
}

//  Return dict of properties for device `did`.
json get_dev_props(vr::IVRSystem* vrsys, vr::TrackedDeviceIndex_t did,
                   vr::ETrackedDeviceClass dclass, int cat, const json& api,
                   bool use_pname, int verb, int ind, int ts)
{
    const auto jverb = g_cfg["verbosity"];
    const auto verr = jverb["error"].get<int>();
    const auto scat = std::to_string(cat);

    json res;
    res["TrackedDeviceClass"] = api["classes"][std::to_string(dclass)];

    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    // abuse vector as a return buffer for the String property
    constexpr size_t BUFFSIZE = 256;
    std::vector<char> buffer(BUFFSIZE);

    for (auto [spid, jname] : api["props"][scat].items()) {
        // convert string to the correct type
        auto pid = static_cast<vr::ETrackedDeviceProperty>(std::stoi(spid));
        // property name
        auto pname = jname.get<std::string>();
        // property type (the last part after '_')
        auto ptype = pname.substr(pname.rfind('_') + 1);
        // property verbosity level (if defined) or max
        int pverb;

        if (jverb["props"].count(pname)) {
            pverb = jverb["props"][pname].get<int>();
        }
        else {
            pverb = jverb["max"].get<int>();
        }

        if (ptype == "Bool") {
            auto pval = vrsys->GetBoolTrackedDeviceProperty(did, pid, &error);
            if (!check_tp_result(vrsys, error, spid, pname, verb, verr, ind, ts)) {
                continue;
            }
            set_tp_val(res, spid, pname, pval, use_pname);
            if (verb >= pverb) {
                prop_head_out(spid, pname, ind, ts);
                std::cout << (pval ? "true" : "false") << '\n';
            }
        }
        else if (ptype == "String") {
            size_t buffsize = vrsys->GetStringTrackedDeviceProperty(
                did, pid, &buffer[0], static_cast<uint32_t>(buffer.size()), &error);
            if (error == vr::TrackedProp_BufferTooSmall) {
                // resize buffer
                buffer.resize(buffsize);
                buffsize = vrsys->GetStringTrackedDeviceProperty(
                    did, pid, &buffer[0], static_cast<uint32_t>(buffer.size()), &error);
            }
            if (!check_tp_result(vrsys, error, spid, pname, verb, verr, ind, ts)) {
                continue;
            }
            set_tp_val(res, spid, pname, &buffer[0], use_pname);
            if (verb >= pverb) {
                prop_head_out(spid, pname, ind, ts);
                std::cout << "\"" << &buffer[0] << "\"" << '\n';
            }
        }
        else if (ptype == "Uint64") {
            auto pval = vrsys->GetUint64TrackedDeviceProperty(did, pid, &error);
            if (!check_tp_result(vrsys, error, spid, pname, verb, verr, ind, ts)) {
                continue;
            }
            set_tp_val(res, spid, pname, pval, use_pname);
            if (verb >= pverb) {
                prop_head_out(spid, pname, ind, ts);
                std::cout << pval << '\n';
            }
        }
        else if (ptype == "Int32") {
            auto pval = vrsys->GetInt32TrackedDeviceProperty(did, pid, &error);
            if (!check_tp_result(vrsys, error, spid, pname, verb, verr, ind, ts)) {
                continue;
            }
            set_tp_val(res, spid, pname, pval, use_pname);
            if (verb >= pverb) {
                prop_head_out(spid, pname, ind, ts);
                std::cout << pval << '\n';
            }
        }
        else if (ptype == "Float") {
            auto pval = vrsys->GetFloatTrackedDeviceProperty(did, pid, &error);
            if (!check_tp_result(vrsys, error, spid, pname, verb, verr, ind, ts)) {
                continue;
            }
            set_tp_val(res, spid, pname, pval, use_pname);
            if (verb >= pverb) {
                prop_head_out(spid, pname, ind, ts);
                std::cout << pval << '\n';
            }
        }
        else if (ptype == "Matrix34") {
            auto pval = vrsys->GetMatrix34TrackedDeviceProperty(did, pid, &error);
            if (!check_tp_result(vrsys, error, spid, pname, verb, verr, ind, ts)) {
                continue;
            }

            std::vector<std::size_t> shape = {3, 4};
            auto mat34 = xt::adapt(&pval.m[0][0], shape);
            set_tp_val_xt(res, spid, pname, mat34, use_pname);

            if (verb >= pverb) {
                prop_head_out(spid, pname, ind, ts);
                std::cout << '\n';
                print_array(mat34, ind + 1, ts);
            }
        }
        else if (ptype == "Array") {
            get_array_type(vrsys, res, did, pid, spid, pname, pverb, use_pname, verb,
                           verr, ind, ts);
        }
        else {
            if (verb >= verr) {
                prop_head_out(spid, pname, ind, ts);
                std::cout << "[property type not implemented]\n";
            }
        }
    }
    return res;
}

//  Return properties for all devices.
json get_all_props(vr::IVRSystem* vrsys, const hdevlist_t& devs, const json& api,
                   bool use_pname, int verb, int ind, int ts)
{
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    const std::string sf(ind * ts, ' ');
    json pvals;

    for (auto [did, dclass] : devs) {
        auto sdid = std::to_string(did);
        auto sdclass = std::to_string(dclass);
        if (verb >= vdef) {
            std::cout << sf << "[" << did << ":"
                      << api["classes"][sdclass].get<std::string>() << "]\n";
        }
        pvals[sdid] = get_dev_props(vrsys, did, dclass, PROP_CAT_COMMON, api, use_pname,
                                    verb, ind + 1, ts);
        if (dclass == vr::TrackedDeviceClass_HMD) {
            pvals[sdid].update(get_dev_props(vrsys, did, dclass, PROP_CAT_HMD, api,
                                             use_pname, verb, ind + 1, ts));
        }
        else if (dclass == vr::TrackedDeviceClass_Controller) {
            pvals[sdid].update(get_dev_props(vrsys, did, dclass, PROP_CAT_CONTROLLER, api,
                                             use_pname, verb, ind + 1, ts));
        }
        else if (dclass == vr::TrackedDeviceClass_TrackingReference) {
            pvals[sdid].update(get_dev_props(vrsys, did, dclass, PROP_CAT_TRACKEDREF, api,
                                             use_pname, verb, ind + 1, ts));
        }
    }
    return pvals;
}

//  Print out the raw (tangent) LRBT values.
void print_raw_lrbt(const json& jd, int ind, int ts)
{
    const std::string sf(ts * ind, ' ');
    constexpr int t1 = 8, t2 = 14, p1 = 6;
    std::cout << std::fixed << std::setprecision(p1);
    std::cout << sf << std::setw(t1) << std::left << "left: " << std::setw(t2)
              << std::right << jd["tan_left"].get<double>() << '\n';
    std::cout << sf << std::setw(t1) << std::left << "right: " << std::setw(t2)
              << std::right << jd["tan_right"].get<double>() << '\n';
    std::cout << sf << std::setw(t1) << std::left << "bottom: " << std::setw(t2)
              << std::right << jd["tan_bottom"].get<double>() << '\n';
    std::cout << sf << std::setw(t1) << std::left << "top: " << std::setw(t2)
              << std::right << jd["tan_top"].get<double>() << '\n';
    std::cout << std::setprecision(6);
}

//  Print single eye FOV values in degrees.
void print_fov(const json& jd, int ind, int ts)
{
    const std::string sf(ts * ind, ' ');
    constexpr int t1 = 8, t2 = 10, p1 = 2;
    std::cout << std::fixed << std::setprecision(p1);
    std::cout << sf << std::setw(t1) << std::left << "left: " << std::setw(t2)
              << std::right << jd["deg_left"].get<double>() << DEG << '\n';
    std::cout << sf << std::setw(t1) << std::left << "right: " << std::setw(t2)
              << std::right << jd["deg_right"].get<double>() << DEG << '\n';
    std::cout << sf << std::setw(t1) << std::left << "bottom: " << std::setw(t2)
              << std::right << jd["deg_bottom"].get<double>() << DEG << '\n';
    std::cout << sf << std::setw(t1) << std::left << "top: " << std::setw(t2)
              << std::right << jd["deg_top"].get<double>() << DEG << '\n';
    std::cout << sf << std::setw(t1) << std::left << "horiz.: " << std::setw(t2)
              << std::right << jd["deg_hor"].get<double>() << DEG << '\n';
    std::cout << sf << std::setw(t1) << std::left << "vert.: " << std::setw(t2)
              << std::right << jd["deg_ver"].get<double>() << DEG << '\n';
    std::cout << std::setprecision(6);
}

//  Print total stereo FOV values in degrees.
void print_fov_total(const json& jd, int ind, int ts)
{
    const std::string sf(ts * ind, ' ');
    constexpr int t1 = 12, t2 = 6, p1 = 2;
    std::cout << std::fixed << std::setprecision(p1);
    std::cout << sf << std::setw(t1) << std::left << "horizontal: " << std::setw(t2)
              << std::right << jd["fov_hor"].get<double>() << DEG << '\n';
    std::cout << sf << std::setw(t1) << std::left << "vertical: " << std::setw(t2)
              << std::right << jd["fov_ver"].get<double>() << DEG << '\n';
    std::cout << sf << std::setw(t1) << std::left << "diagonal: " << std::setw(t2)
              << std::right << jd["fov_diag"].get<double>() << DEG << '\n';
    std::cout << sf << std::setw(t1) << std::left << "overlap: " << std::setw(t2)
              << std::right << jd["overlap"].get<double>() << DEG << '\n';
    std::cout << std::setprecision(6);
}

//  Print view geometry (panel rotation, IPD).
void print_view_geom(const json& jd, int ind, int ts)
{
    const std::string sf(ts * ind, ' ');
    constexpr int t1 = 22, t2 = 6, p1 = 1;
    std::cout << std::fixed << std::setprecision(p1);
    std::cout << sf << std::setw(t1) << std::left
              << "left panel rotation: " << std::setw(t2) << std::right
              << jd["left_rot"].get<double>() << DEG << '\n';
    std::cout << sf << std::setw(t1) << std::left
              << "right panel rotation: " << std::setw(t2) << std::right
              << jd["right_rot"].get<double>() << DEG << '\n';
    std::cout << sf << std::setw(t1) << std::left << "reported IPD: " << std::setw(t2)
              << std::right << jd["ipd"].get<double>() << MM << '\n';
    std::cout << std::setprecision(6);
}

//  Print the hidden area mask mesh statistics.
void print_ham_mesh(const json& ham_mesh, const char* neye, int verb, int vgeom, int ind,
                    int ts)
{
    const std::string sf(ts * ind, ' ');
    const std::string sf1(ts * (ind + 1), ' ');
    const int t1 = 20;

    std::cout << sf << neye << " eye HAM mesh:\n";
    if (!ham_mesh.is_null()) {
        auto nverts = ham_mesh["verts_raw"].size();
        // just a safety check that the data are authentic
        HMDQ_ASSERT(nverts % 3 == 0);
        auto nfaces = nverts / 3;
        auto nverts_opt = ham_mesh["verts_opt"].size();
        auto nfaces_opt = ham_mesh["faces_opt"].size();
        auto ham_area = ham_mesh["ham_area"].get<double>();

        std::cout << sf1 << std::setw(t1) << "original vertices: " << nverts
                  << ", triangles: " << nfaces << '\n';
        if (verb >= vgeom) {
            std::cout << sf1 << std::setw(t1) << "optimized vertices: " << nverts_opt
                      << ", n-gons: " << nfaces_opt << '\n';
        }
        std::cout << sf1 << std::setw(t1) << "mesh area: " << std::fixed
                  << std::setprecision(2) << ham_area * 100 << PRCT << "\n";
    }
    else {
        std::cout << sf1 << "No mesh defined by the headset\n";
    }
}

//  Enumerate view and projection geometry for both eyes.
json get_eyes_geometry(vr::IVRSystem* vrsys, const heyes_t& eyes, int verb, int ind,
                       int ts)
{
    const auto vdef = g_cfg["verbosity"]["default"].get<int>();
    const auto vgeom = g_cfg["verbosity"]["geom"].get<int>();
    const std::string sf(ts * ind, ' ');
    const std::string sf1(ts * (ind + 1), ' ');

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
    if (verb >= vdef) {
        std::cout << sf << "Recommended render target size: " << rec_rts << "\n\n";
    }

    for (auto [eye, neye] : eyes) {

        ham_mesh[neye] = get_ham_mesh_opt(vrsys, eye, vr::k_eHiddenAreaMesh_Standard);
        if (verb >= vdef) {
            print_ham_mesh(ham_mesh[neye], neye.c_str(), verb, vgeom, ind, ts);
            std::cout << '\n';
        }

        auto oe2h = vrsys->GetEyeToHeadTransform(eye);
        std::vector<std::size_t> shape = {3, 4};
        auto e2h = xt::adapt(&oe2h.m[0][0], shape);
        eye2head[neye] = e2h;
        if (verb >= vgeom) {
            std::cout << sf << neye << " eye to head transformation matrix:\n";
            print_array(e2h, ind + 1, ts);
            std::cout << '\n';
        }

        raw_eye[neye] = get_raw_eye(vrsys, eye);
        if (verb >= vgeom) {
            std::cout << sf << neye << " eye raw LRBT values:\n";
            print_raw_lrbt(raw_eye[neye], ind + 1, ts);
            std::cout << '\n';
        }

        // build eye FOV points only if eye FOV is different from head FOV
        if (xt::view(e2h, xt::all(), xt::range(0, 3)) != xt::eye<double>(3, 0)) {
            fov_eye[neye] = get_fov(raw_eye[neye], ham_mesh[neye]);
            if (verb >= vdef) {
                std::cout << sf << neye << " eye raw FOV:\n";
                print_fov(fov_eye[neye], ind + 1, ts);
                std::cout << '\n';
            }
        }

        harray2d_t rot = xt::view(e2h, xt::all(), xt::range(0, 3));
        fov_head[neye] = get_fov(raw_eye[neye], ham_mesh[neye], &rot);
        if (verb >= vdef) {
            std::cout << sf << neye << " eye head FOV:\n";
            print_fov(fov_head[neye], ind + 1, ts);
            std::cout << '\n';
        }
    }

    auto fov_tot = get_total_fov(fov_head);
    if (verb >= vdef) {
        std::cout << "Total FOV:\n";
        print_fov_total(fov_tot, ind + 1, ts);
        std::cout << '\n';
    }

    auto view_geom = get_view_geom(eye2head);
    if (verb >= vdef) {
        std::cout << sf << "View geometry:\n";
        print_view_geom(view_geom, ind + 1, ts);
    }

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
    const std::string sf(ts * ind, ' ');

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
                std::cout << sf << "OpenVR runtime path: " << &buffer[0] << '\n';
            }
        }
        else {
            std::cerr << sf << "Error: Cannot get the runtime path\n";
            return nullptr;
        }
    }
    else {
        std::cerr << sf << "Error: No OpenVR runtime found\n";
        return nullptr;
    }

    if (!vr::VR_IsHmdPresent()) {
        std::cerr << sf << "Error: No HMD found\n";
        return nullptr;
    }

    vr::EVRInitError eError = vr::VRInitError_None;
    vr::IVRSystem* vrsys = vr::VR_Init(&eError, app_type);

    if (eError != vr::VRInitError_None) {
        vrsys = NULL;
        std::cerr << sf << "Error: " << vr::VR_GetVRInitErrorAsEnglishDescription(eError)
                  << '\n';
        return nullptr;
    }
    return vrsys;
}
