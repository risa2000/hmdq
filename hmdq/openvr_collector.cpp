/******************************************************************************
 * HMDQ Tools - tools for an OpenVR HMD and other hardware introspection      *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2020, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#include <algorithm>
#include <filesystem>
#include <tuple>

#include <fmt/format.h>

#define OPENVR_BUILD_STATIC
#include <openvr/openvr.h>

#include <xtensor/xadapt.hpp>
#include <xtensor/xjson.hpp>
#include <xtensor/xview.hpp>

#include "base_common.h"
#include "except.h"
#include "jkeys.h"
#include "jtools.h"
#include "openvr_collector.h"
#include "openvr_common.h"
#include "xtdef.h"

#include "fifo_map_fix.h"

namespace openvr {

//  local constants
//------------------------------------------------------------------------------
static constexpr size_t BUFFSIZE = 256;

static const int PROP_CAT_COMMON = 1;
static const int PROP_CAT_HMD = 2;
static const int PROP_CAT_CONTROLLER = 3;
static const int PROP_CAT_TRACKEDREF = 4;

//  helper (local) functions for OpenVR collector
//------------------------------------------------------------------------------
//  Return OpenVR runtime path.
std::filesystem::path get_runtime_path()
{
    constexpr size_t cbuffsize = BUFFSIZE;
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
        return std::filesystem::u8path(&buffer[0]);
    }
    else {
        return "";
    }
}

//  Initialize OpenVR subsystem and return IVRSystem interace.
std::tuple<vr::IVRSystem*, vr::EVRInitError> init_vrsys(vr::EVRApplicationType app_type)
{
    vr::EVRInitError eError = vr::VRInitError_None;
    vr::IVRSystem* vrsys = vr::VR_Init(&eError, app_type);

    if (eError != vr::VRInitError_None) {
        return {nullptr, eError};
    }
    return {vrsys, eError};
}

//  Return OpenVR version from the runtime.
const char* get_runtime_ver(vr::IVRSystem* vrsys)
{
    return vrsys->GetRuntimeVersion();
}

//  Enumerate the attached devices.
hdevlist_t enum_devs(vr::IVRSystem* vrsys)
{
    hdevlist_t res;
    for (vr::TrackedDeviceIndex_t dev_id = 0; dev_id < vr::k_unMaxTrackedDeviceCount;
         ++dev_id) {
        const auto dev_class = vrsys->GetTrackedDeviceClass(dev_id);
        if (dev_class != vr::TrackedDeviceClass_Invalid) {
            res.push_back(std::make_pair(dev_id, dev_class));
        }
    }
    return res;
}

//  Check the returned value and print out the error message if detected.
inline bool check_tp_result(vr::IVRSystem* vrsys, json& jd, const std::string& pname,
                            vr::ETrackedPropertyError res)
{
    if (res == vr::TrackedProp_Success) {
        return true;
    }
    const auto msg = fmt::format("{:s}", vrsys->GetPropErrorNameFromEnum(res));
    jd[pname][ERROR_PREFIX] = msg;
    return false;
}

//  Set TrackedPropertyValue (vector value) in the JSON dict.
template<typename T>
void set_tp_val_1d_array(json& jd, const std::string& pname,
                         const std::vector<unsigned char>& buffer, size_t buffsize)
{
    // we got float array in buffer of buffsize / sizeof(float)
    const auto ptype = reinterpret_cast<const T*>(&buffer[0]);
    const auto size = buffsize / sizeof(T);
    const std::vector<std::size_t> shape = {size};
    const xt::xtensor<T, 1> atype = xt::adapt(ptype, size, xt::no_ownership(), shape);
    jd[pname] = atype;
}

//  Set TrackedPropertyValue (matrix array value) in the JSON dict.
template<typename M>
void set_tp_val_mat_array(json& jd, const std::string& pname,
                          const std::vector<unsigned char>& buffer, size_t buffsize)
{
    // we got float array in buffer of buffsize / sizeof(float)
    const auto pfloat = reinterpret_cast<const float*>(&buffer[0]);
    const auto size = buffsize / sizeof(float);
    constexpr auto nrows = sizeof(M::m) / sizeof(M::m[0]);
    constexpr auto ncols = sizeof(M::m[0]) / sizeof(float);
    const std::vector<std::size_t> shape = {buffsize / sizeof(M::m), nrows, ncols};
    const harray_t amats = xt::adapt(pfloat, size, xt::no_ownership(), shape);
    jd[pname] = amats;
}

//  Set TrackedPropertyValue (vector array value) in the JSON dict.
template<typename V, typename I = float>
void set_tp_val_vec_array(json& jd, const std::string& pname,
                          const std::vector<unsigned char>& buffer, size_t buffsize)
{
    // we got I type array in buffer of buffsize / sizeof(I)
    const auto pitem = reinterpret_cast<const I*>(&buffer[0]);
    const auto size = buffsize / sizeof(I);
    constexpr auto vsize = sizeof(V::v) / sizeof(I);
    const std::vector<std::size_t> shape = {buffsize / sizeof(V::v), vsize};
    const harray_t avecs = xt::adapt(pitem, size, xt::no_ownership(), shape);
    jd[pname] = avecs;
}

//  Universal routine to get any "Array" property into the JSON dict.
void get_array_type(vr::IVRSystem* vrsys, json& jd, vr::TrackedDeviceIndex_t did,
                    vr::ETrackedDeviceProperty pid, const std::string& pname)
{
    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    // abuse vector as a return buffer for an Array property
    std::vector<unsigned char> buffer(BUFFSIZE);
    // parse the name to get the type
    const auto [basename, ptype_name, ptype, is_array] = basevr::parse_prop_name(pname);

    if (ptype == basevr::PropType::Invalid) {
        const auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype_name);
        jd[pname][ERROR_PREFIX] = msg;
        return;
    }

    vr::PropertyTypeTag_t ptag = ptype_to_ptag(ptype);
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

    switch (ptype) {
        case basevr::PropType::Float:
            set_tp_val_1d_array<float>(jd, pname, buffer, buffsize);
            break;
        case basevr::PropType::Int32:
            set_tp_val_1d_array<int32_t>(jd, pname, buffer, buffsize);
            break;
        case basevr::PropType::Uint64:
            set_tp_val_1d_array<uint64_t>(jd, pname, buffer, buffsize);
            break;
        case basevr::PropType::Bool:
            set_tp_val_1d_array<bool>(jd, pname, buffer, buffsize);
            break;
        case basevr::PropType::Matrix34:
            set_tp_val_mat_array<vr::HmdMatrix34_t>(jd, pname, buffer, buffsize);
            break;
        case basevr::PropType::Matrix44:
            set_tp_val_mat_array<vr::HmdMatrix44_t>(jd, pname, buffer, buffsize);
            break;
        case basevr::PropType::Vector2:
            set_tp_val_vec_array<vr::HmdVector2_t>(jd, pname, buffer, buffsize);
            break;
        case basevr::PropType::Vector3:
            set_tp_val_vec_array<vr::HmdVector3_t>(jd, pname, buffer, buffsize);
            break;
        case basevr::PropType::Vector4:
            set_tp_val_vec_array<vr::HmdVector4_t>(jd, pname, buffer, buffsize);
            break;
        default:
            const auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype_name);
            jd[pname][ERROR_PREFIX] = msg;
            break;
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

//  Return dict of properties for device `did`.
json get_dev_props(vr::IVRSystem* vrsys, vr::TrackedDeviceIndex_t did,
                   vr::ETrackedDeviceClass dclass, int cat, const json& api)
{
    const auto scat = std::to_string(cat);

    json res;

    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    // abuse vector as a return buffer for the String property
    std::vector<char> buffer(BUFFSIZE);

    for (const auto& [spid, jname] : api[j_properties][scat].items()) {
        // convert string to the correct type
        const auto pid = static_cast<vr::ETrackedDeviceProperty>(std::stoi(spid));
        // property name
        const auto pname = jname.get<std::string>();
        // parse the name to get the type
        const auto [basename, ptype_name, ptype, is_array]
            = basevr::parse_prop_name(pname);

        if (is_array) {
            get_array_type(vrsys, res, did, pid, pname);
            continue;
        }
        switch (ptype) {
            case basevr::PropType::Bool: {
                const auto pval = vrsys->GetBoolTrackedDeviceProperty(did, pid, &error);
                if (check_tp_result(vrsys, res, pname, error)) {
                    res[pname] = pval;
                }
                break;
            }
            case basevr::PropType::String: {
                if (get_str_tracked_prop(vrsys, res, did, pid, pname, buffer)) {
                    res[pname] = &buffer[0];
                }
                break;
            }
            case basevr::PropType::Uint64: {
                const auto pval = vrsys->GetUint64TrackedDeviceProperty(did, pid, &error);
                if (check_tp_result(vrsys, res, pname, error)) {
                    res[pname] = pval;
                }
                break;
            }
            case basevr::PropType::Int32: {
                const auto pval = vrsys->GetInt32TrackedDeviceProperty(did, pid, &error);
                if (check_tp_result(vrsys, res, pname, error)) {
                    res[pname] = pval;
                }
                break;
            }
            case basevr::PropType::Float: {
                const auto pval = vrsys->GetFloatTrackedDeviceProperty(did, pid, &error);
                if (check_tp_result(vrsys, res, pname, error)) {
                    res[pname] = pval;
                }
                break;
            }
            case basevr::PropType::Vector2:
            case basevr::PropType::Vector3:
            case basevr::PropType::Vector4:
            case basevr::PropType::Matrix34:
            case basevr::PropType::Matrix44: {
                // get not directly supported types as an array of one item
                get_array_type(vrsys, res, did, pid, pname);
                // use this trick to remove surrogate "array" from the JSON if there was
                // no error
                if (res[pname].find(ERROR_PREFIX) == res[pname].end())
                    res[pname] = res[pname][0];
                break;
            }
            default: {
                const auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype_name);
                res[pname][ERROR_PREFIX] = msg;
                break;
            }
        }
    }
    return res;
}

//  Return properties for all devices.
json get_all_props(vr::IVRSystem* vrsys, const hdevlist_t& devs, const json& api)
{
    json pvals;

    for (const auto [did, dclass] : devs) {
        const auto sdid = std::to_string(did);
        pvals[sdid] = get_dev_props(vrsys, did, dclass, PROP_CAT_COMMON, api);
        if (dclass == vr::TrackedDeviceClass_HMD) {
            pvals[sdid].update(get_dev_props(vrsys, did, dclass, PROP_CAT_HMD, api));
        }
        else if (dclass == vr::TrackedDeviceClass_Controller) {
            pvals[sdid].update(
                get_dev_props(vrsys, did, dclass, PROP_CAT_CONTROLLER, api));
        }
        else if (dclass == vr::TrackedDeviceClass_TrackingReference) {
            pvals[sdid].update(
                get_dev_props(vrsys, did, dclass, PROP_CAT_TRACKEDREF, api));
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
    const auto size = hmesh.unTriangleCount * 3 * dim;
    const std::vector<std::size_t> shape
        = {static_cast<size_t>(hmesh.unTriangleCount) * 3, dim};
    const harray2d_t lverts
        = xt::adapt(&hmesh.pVertexData[0].v[0], size, xt::no_ownership(), shape);
    return lverts;
}

//  Get hidden area mask (HAM) mesh.
json get_ham_mesh(vr::IVRSystem* vrsys, vr::EVREye eye, vr::EHiddenAreaMeshType hamtype)
{
    const auto hmesh = vrsys->GetHiddenAreaMesh(eye, hamtype);
    if (hmesh.unTriangleCount == 0) {
        return json();
    }
    const auto verts = hmesh2np(hmesh);
    // number of vertices must be divisible by 3 as each 3 defined one triangle
    HMDQ_ASSERT(verts.shape(0) % 3 == 0);
    json res;
    res[j_verts_raw] = verts;
    return res;
}

//  Get raw projection values (LRBT) for `eye`.
json get_raw_eye(vr::IVRSystem* vrsys, vr::EVREye eye)
{
    float left, right, bottom, top;
    // NOTE: the API doc has swapped values for top and bottom
    vrsys->GetProjectionRaw(eye, &left, &right, &bottom, &top);
    const auto aspect = (right - left) / (top - bottom);
    json res = {{j_tan_left, left},
                {j_tan_right, right},
                {j_tan_bottom, bottom},
                {j_tan_top, top},
                {j_aspect, aspect}};
    return res;
}

//  Get eye to head transform matrix.
json get_eye2head(vr::IVRSystem* vrsys, vr::EVREye eye)
{
    // get eye to head transformation matrix
    const auto oe2h = vrsys->GetEyeToHeadTransform(eye);
    const auto e2h = xt::adapt(&oe2h.m[0][0], {3, 4});
    json je2h = e2h;
    return je2h;
}

//  Enumerate view and projection geometry for both eyes.
json get_geometry(vr::IVRSystem* vrsys)
{
    // all the data are collected into specific `json`s
    json eye2head;
    json raw_eye;
    json ham_mesh;

    uint32_t rec_width, rec_height;
    vrsys->GetRecommendedRenderTargetSize(&rec_width, &rec_height);
    std::vector<uint32_t> rec_rts = {rec_width, rec_height};

    for (const auto& [eye, neye] : EYES) {

        // get HAM mesh (if supported by the headset, otherwise 'null')
        ham_mesh[neye] = get_ham_mesh(vrsys, eye, vr::k_eHiddenAreaMesh_Standard);

        // get eye to head transformation matrix
        eye2head[neye] = get_eye2head(vrsys, eye);

        // get raw eye values (direct from OpenVR)
        raw_eye[neye] = get_raw_eye(vrsys, eye);
    }

    json res;
    res[j_rec_rts] = rec_rts;
    res[j_raw_eye] = raw_eye;
    res[j_eye2head] = eye2head;
    res[j_ham_mesh] = ham_mesh;

    return res;
}

//  Return some info about OpenVR.
json get_openvr(vr::IVRSystem* vrsys, const json& api)
{
    json res;
    res[j_rt_path] = get_runtime_path().u8string();
    res[j_rt_ver] = get_runtime_ver(vrsys);

    const hdevlist_t devs = enum_devs(vrsys);
    if (devs.size()) {
        res[j_devices] = devs;
        // get all the properties
        res[j_properties] = get_all_props(vrsys, devs, api);
        // record geometry only if HMD device class is present
        // this technically should be always true, unless the user explicitly requested
        // running OpenVR without a HMD.
        if (devs.end() != std::find_if(devs.begin(), devs.end(), [](auto p) {
                return p.second == vr::TrackedDeviceClass_HMD;
            })) {
            // get all the geometry
            res[j_geometry] = get_geometry(vrsys);
        }
    }

    return res;
}

//  OpenVR Collector class
//------------------------------------------------------------------------------
Collector::~Collector()
{
    shutdown();
}

// Return API extract (for printer)
const json& Collector::get_xapi()
{
    return m_jApi;
}

// Shutdown the OpenVR subsystem
void Collector::shutdown()
{
    if (nullptr != m_ivrSystem) {
        vr::VR_Shutdown();
        m_ivrSystem = nullptr;
    }
}

// Check if the OpenVR subsystem is present and initialize it
// Return: true if present and initialized, otherwise false
bool Collector::try_init()
{
    if (!vr::VR_IsRuntimeInstalled()) {
        m_err = vr::VRInitError_Init_InstallationNotFound;
        return false;
    }
    bool res = false;
    auto [vrsys, error] = init_vrsys(m_appType);
    if (nullptr != vrsys) {
        m_ivrSystem = vrsys;
        res = true;
        json oapi = read_json(m_apiPath);
        m_jApi = parse_json_oapi(oapi);
    }
    m_err = error;
    return res;
}

// Collect the OpenVR subsystem data
void Collector::collect()
{
    m_jData = get_openvr(m_ivrSystem, m_jApi);
}

// Return OpenVR subystem ID
std::string Collector::get_id()
{
    return j_openvr;
}

// Return the last OpenVR subsystem error
int Collector::get_last_error()
{
    return static_cast<int>(m_err);
}

// Return the last OpenVR subsystem error message
std::string Collector::get_last_error_msg()
{
    return std::string(vr::VR_GetVRInitErrorAsEnglishDescription(m_err));
}

// Return OpenVR data
json& Collector::get_data()
{
    return m_jData;
}

} // namespace openvr
