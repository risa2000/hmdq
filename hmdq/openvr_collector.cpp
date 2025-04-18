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

#include "openvr_collector.h"
#include "base_common.h"
#include "except.h"
#include "jkeys.h"
#include "json_proxy.h"
#include "jtools.h"
#include "openvr_common.h"
#include "wintools.h"
#include "xtdef.h"

#include <xtensor/containers/xadapt.hpp>
#include <xtensor/io/xjson.hpp>
#include <xtensor/views/xview.hpp>

#include <openvr/openvr.h>

#include <fmt/format.h>

#include <algorithm>
#include <filesystem>
#include <tuple>
#include <type_traits>

namespace openvr {

//  local constants
//------------------------------------------------------------------------------
static constexpr size_t BUFFSIZE = 256;

static const int PROP_CAT_COMMON = 1;
static const int PROP_CAT_HMD = 2;
static const int PROP_CAT_CONTROLLER = 3;
static const int PROP_CAT_TRACKEDREF = 4;
static const int PROP_CAT_UI = 5;
static const int PROP_CAT_UI_MIN = vr::Prop_IconPathName_String;
static const int PROP_CAT_UI_MAX = vr::Prop_DisplayHiddenArea_Binary_Start;
static const int PROP_CAT_DRIVER = 6;
static const int PROP_CAT_INTERNAL = 7;

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
        return utf8_to_path(&buffer[0]);
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
inline json get_tp_error(vr::IVRSystem* vrsys, vr::ETrackedPropertyError err)
{
    const auto msg = fmt::format("{:s}", vrsys->GetPropErrorNameFromEnum(err));
    return make_error_obj(msg);
}

//  Get vector of scalar values (integral types) into a JSON dict.
template<typename T>
json get_val_1d_array(const std::vector<unsigned char>& buffer, size_t buffsize)
{
    const auto ptype = reinterpret_cast<const T*>(&buffer[0]);
    const auto size = buffsize / sizeof(T);
    const std::vector<std::size_t> shape = {size};
    return xt::adapt(ptype, size, xt::no_ownership(), shape);
}

//  Get vector of vector values into a JSON dict.
template<typename T, size_t ncount>
json get_val_vec_array(const std::vector<unsigned char>& buffer, size_t buffsize)
{
    const auto pitem = reinterpret_cast<const T*>(&buffer[0]);
    const auto size = buffsize / sizeof(T);
    constexpr auto vecsize = sizeof(T) * ncount;
    const std::vector<std::size_t> shape = {buffsize / vecsize, ncount};
    return xt::adapt(pitem, size, xt::no_ownership(), shape);
}

//  Get vector of matrix values into a JSON dict.
template<typename T, size_t nrows, size_t ncols>
json get_val_mat_array(const std::vector<unsigned char>& buffer, size_t buffsize)
{
    const auto pcell = reinterpret_cast<const T*>(&buffer[0]);
    const auto size = buffsize / sizeof(T);
    constexpr auto matsize = sizeof(T) * nrows * ncols;
    const std::vector<std::size_t> shape = {buffsize / matsize, nrows, ncols};
    return xt::adapt(pcell, size, xt::no_ownership(), shape);
}

//  Get vector of vr::HmdVector*_t into a JSON dict.
template<typename V>
json get_val_vec_array(const std::vector<unsigned char>& buffer, size_t buffsize)
{
    using scalar_t = typename std::remove_all_extents<decltype(V::v)>::type;
    constexpr auto vdim = sizeof(V::v) / sizeof(scalar_t);
    return get_val_vec_array<scalar_t, vdim>(buffer, buffsize);
}

//  Get vector of vr::HmdMatrix**_t into a JSON dict.
template<typename M>
json get_val_mat_array(const std::vector<unsigned char>& buffer, size_t buffsize)
{
    using scalar_t = typename std::remove_all_extents<decltype(M::m)>::type;
    constexpr auto nrows = sizeof(M::m) / sizeof(M::m[0]);
    constexpr auto ncols = sizeof(M::m[0]) / sizeof(scalar_t);
    return get_val_mat_array<scalar_t, nrows, ncols>(buffer, buffsize);
}

//  Get array of <prop_name> type into a JSON dict.
json prop_array_to_json(const std::string& pname,
                        const std::vector<unsigned char>& buffer)
{
    // parse the name to get the type
    const auto [basename, ptype_name, ptype, is_array] = basevr::parse_prop_name(pname);

    switch (ptype) {
        case basevr::PropType::Bool:
            return get_val_1d_array<bool>(buffer, buffer.size());
        case basevr::PropType::Float:
            return get_val_1d_array<float>(buffer, buffer.size());
        case basevr::PropType::Double:
            return get_val_1d_array<double>(buffer, buffer.size());
        case basevr::PropType::Int16:
            return get_val_1d_array<int16_t>(buffer, buffer.size());
        case basevr::PropType::Uint16:
            return get_val_1d_array<uint16_t>(buffer, buffer.size());
        case basevr::PropType::Int32:
            return get_val_1d_array<int32_t>(buffer, buffer.size());
        case basevr::PropType::Uint32:
            return get_val_1d_array<uint32_t>(buffer, buffer.size());
        case basevr::PropType::Int64:
            return get_val_1d_array<int64_t>(buffer, buffer.size());
        case basevr::PropType::Uint64:
            return get_val_1d_array<uint64_t>(buffer, buffer.size());
        case basevr::PropType::Matrix34:
            return get_val_mat_array<vr::HmdMatrix34_t>(buffer, buffer.size());
        case basevr::PropType::Matrix44:
            return get_val_mat_array<vr::HmdMatrix44_t>(buffer, buffer.size());
        case basevr::PropType::Vector2:
            return get_val_vec_array<vr::HmdVector2_t>(buffer, buffer.size());
        case basevr::PropType::Vector3:
            return get_val_vec_array<vr::HmdVector3_t>(buffer, buffer.size());
        case basevr::PropType::Vector4:
            return get_val_vec_array<vr::HmdVector4_t>(buffer, buffer.size());
        default:
            const auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype_name);
            return make_error_obj(msg);
    }
}

//  Get array tracked property, or scalar property via an array interface.
std::vector<unsigned char>
get_array_tracked_prop(vr::IVRSystem* vrsys, vr::TrackedDeviceIndex_t did,
                       vr::ETrackedDeviceProperty pid, vr::PropertyTypeTag_t ptag,
                       vr::ETrackedPropertyError* pError = nullptr)
{
    // abuse vector as a return buffer for an Array property
    std::vector<unsigned char> buffer(BUFFSIZE);
    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    size_t buffsize = vrsys->GetArrayTrackedDeviceProperty(
        did, pid, ptag, &buffer[0], static_cast<uint32_t>(buffer.size()), &error);
    if (error == vr::TrackedProp_BufferTooSmall) {
        // resize buffer
        buffer.resize(buffsize);
        buffsize = vrsys->GetArrayTrackedDeviceProperty(
            did, pid, ptag, &buffer[0], static_cast<uint32_t>(buffer.size()), &error);
    }
    if (nullptr != pError) {
        *pError = error;
    }
    if (vr::TrackedProp_Success == error) {
        buffer.resize(buffsize);
        return buffer;
    }
    else {
        return {};
    }
}

//  Universal routine to get any scalar or array property into the JSON dict.
json get_any_type_prop(vr::IVRSystem* vrsys, vr::TrackedDeviceIndex_t did,
                       vr::ETrackedDeviceProperty pid, const std::string& pname)
{
    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    // parse the name to get the type
    const auto [basename, ptype_name, ptype, is_array] = basevr::parse_prop_name(pname);

    if (ptype == basevr::PropType::Invalid) {
        const auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype_name);
        return make_error_obj(msg);
    }

    vr::PropertyTypeTag_t ptag = ptype_to_ptag(ptype);
    std::vector<unsigned char> aval
        = get_array_tracked_prop(vrsys, did, pid, ptag, &error);

    if (vr::TrackedProp_Success != error) {
        return get_tp_error(vrsys, error);
    }

    if (ptype == basevr::PropType::String) {
        // for String type interpret directly the buffer as a string
        return reinterpret_cast<char*>(&aval[0]);
    }

    json temp = prop_array_to_json(pname, aval);
    if (is_array) {
        return temp;
    }
    else {
        // if not dealing with an array property "remove" the array (brackets)
        return temp[0];
    }
}

//  Return dict of properties for device `did` in the range
json get_dev_props_range(vr::IVRSystem* vrsys, vr::TrackedDeviceIndex_t did,
                         vr::ETrackedDeviceClass dclass, int cat, int min_pid,
                         int max_pid, const json& api)
{
    const auto scat = std::to_string(cat);
    json res;

    vr::ETrackedPropertyError error = vr::TrackedProp_Success;
    for (const auto& [spid, jname] : api[j_properties][scat].items()) {
        // convert string to the correct type
        const auto pid = static_cast<vr::ETrackedDeviceProperty>(std::stoi(spid));
        if (pid < min_pid || pid >= max_pid) {
            continue;
        }
        // property name
        const auto pname = jname.get<std::string>();
        // use all-in-one matic function
        res[pname] = get_any_type_prop(vrsys, did, pid, pname);
    }
    return res;
}

//  Return dict of properties for device `did`.
json get_dev_props(vr::IVRSystem* vrsys, vr::TrackedDeviceIndex_t did,
                   vr::ETrackedDeviceClass dclass, int cat, const json& api)
{
    return get_dev_props_range(vrsys, did, dclass, cat, cat * 1000, (cat + 1) * 1000,
                               api);
}

//  Return properties for all devices.
json get_all_props(vr::IVRSystem* vrsys, const hdevlist_t& devs, const json& api)
{
    json pvals;

    for (const auto& [did, dclass] : devs) {
        const auto sdid = std::to_string(did);
        pvals[sdid] = get_dev_props(vrsys, did, dclass, PROP_CAT_COMMON, api);
        if (dclass == vr::TrackedDeviceClass_HMD) {
            pvals[sdid].update(get_dev_props(vrsys, did, dclass, PROP_CAT_HMD, api));
            pvals[sdid].update(get_dev_props_range(
                vrsys, did, dclass, PROP_CAT_UI, PROP_CAT_UI_MIN, PROP_CAT_UI_MAX, api));
            pvals[sdid].update(get_dev_props(vrsys, did, dclass, PROP_CAT_DRIVER, api));
            pvals[sdid].update(get_dev_props(vrsys, did, dclass, PROP_CAT_INTERNAL, api));
        }
        else if (dclass == vr::TrackedDeviceClass_Controller) {
            pvals[sdid].update(
                get_dev_props(vrsys, did, dclass, PROP_CAT_CONTROLLER, api));
            pvals[sdid].update(get_dev_props_range(
                vrsys, did, dclass, PROP_CAT_UI, PROP_CAT_UI_MIN, PROP_CAT_UI_MAX, api));
            pvals[sdid].update(get_dev_props(vrsys, did, dclass, PROP_CAT_INTERNAL, api));
        }
        else if (dclass == vr::TrackedDeviceClass_TrackingReference) {
            pvals[sdid].update(
                get_dev_props(vrsys, did, dclass, PROP_CAT_TRACKEDREF, api));
            pvals[sdid].update(get_dev_props_range(
                vrsys, did, dclass, PROP_CAT_UI, PROP_CAT_UI_MIN, PROP_CAT_UI_MAX, api));
            pvals[sdid].update(get_dev_props(vrsys, did, dclass, PROP_CAT_INTERNAL, api));
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
    res[j_rt_path] = get_runtime_path().string();
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
std::shared_ptr<json> Collector::get_xapi()
{
    return m_pjApi;
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
        add_error(*m_pjData, get_last_error_msg());
        return false;
    }
    bool res = false;
    auto [vrsys, error] = init_vrsys(m_appType);
    m_err = error;
    if (nullptr != vrsys) {
        m_ivrSystem = vrsys;
        json oapi = read_json(m_apiPath);
        *m_pjApi = parse_json_oapi(oapi);
        res = true;
    }
    else {
        add_error(*m_pjData, get_last_error_msg());
    }
    return res;
}

// Collect the OpenVR subsystem data
void Collector::collect()
{
    *m_pjData = get_openvr(m_ivrSystem, *m_pjApi);
}

// Return the last OpenVR subsystem error
int Collector::get_last_error() const
{
    return static_cast<int>(m_err);
}

// Return the last OpenVR subsystem error message
std::string Collector::get_last_error_msg() const
{
    return std::string(vr::VR_GetVRInitErrorAsEnglishDescription(m_err));
}

} // namespace openvr
