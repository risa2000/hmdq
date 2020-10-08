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

#include "base_common.h"
#include "config.h"
#include "fmthlp.h"
#include "jkeys.h"
#include "jtools.h"
#include "xtdef.h"

#include <xtensor/xjson.hpp>

namespace basevr {

//  generic functions
//------------------------------------------------------------------------------
//  Resolve property tag enum from the type name.
PropType ptype_from_ptypename(const std::string& ptype_name)
{
    static const std::map<std::string, PropType> ptype_map = {
        {"Float", PropType::Float},
        {"Double", PropType::Double},

        {"Int16", PropType::Int16},
        {"Uint16", PropType::Uint16},
        {"Int32", PropType::Int32},
        {"Uint32", PropType::Uint32},
        {"Int64", PropType::Int64},
        {"Uint64", PropType::Uint64},

        {"Bool", PropType::Bool},
        {"String", PropType::String},

        {"Vector2", PropType::Vector2},
        {"Vector3", PropType::Vector3},
        {"Vector4", PropType::Vector4},

        {"Matrix33", PropType::Matrix33},
        {"Matrix34", PropType::Matrix34},
        {"Matrix44", PropType::Matrix44},

        {"Quaternion", PropType::Quaternion},

        {"Quad", PropType::Quad},
    };
    auto res = ptype_map.find(ptype_name);
    if (res != ptype_map.end()) {
        return res->second;
    }
    else {
        return PropType::Invalid;
    }
}

//  Return {<str:base_name>, <str:type_name>, <enum:type>, <bool:array>}
std::tuple<std::string, std::string, PropType, bool>
parse_prop_name(const std::string& pname)
{
    bool is_array = false;
    // cutting out the prefix
    const auto lpos1 = pname.find('_');
    // property type (the last part after '_')
    auto rpos1 = pname.rfind('_');
    auto ptype = pname.substr(rpos1 + 1);
    if (ptype == "Array") {
        const auto rpos2 = pname.rfind('_', rpos1 - 1);
        ptype = pname.substr(rpos2 + 1, rpos1 - rpos2 - 1);
        rpos1 = rpos2;
        is_array = true;
    }
    const auto basename = pname.substr(lpos1 + 1, rpos1 - lpos1 - 1);
    const auto type_name = pname.substr(rpos1 + 1);
    return {basename, type_name, ptype_from_ptypename(ptype), is_array};
}

//  Print property functions.
//------------------------------------------------------------------------------
//  Print the property name to stdout.
inline void prop_head_out(int pid, const std::string& name, bool is_array, int ind,
                          int ts)
{
    if (pid >= 1000) {
        iprint(ind * ts, "{:4d} : {:s}{:s} = ", pid, name, is_array ? "[]" : "");
    }
    else if (pid >= 0) {
        iprint(ind * ts, "{:2d} : {:s}{:s} = ", pid, name, is_array ? "[]" : "");
    }
    else {
        iprint(ind * ts, "{:s}{:s} = ", name, is_array ? "[]" : "");
    }
}

//  Print (non-error) value of an Array type property.
void print_array_type(const std::string& pname, const json& pval, int ind, int ts)
{
    const auto sf = ind * ts;
    // parse the name to get the type
    const auto [basename, ptype_name, ptype, is_array] = parse_prop_name(pname);

    switch (ptype) {
        case PropType::Float:
            print_tensor<double, 1>(pval.get<hvector_t>(), ind, ts);
            break;
        case PropType::Int32:
            print_tensor<int32_t, 1>(pval.get<xt::xtensor<int32_t, 1>>(), ind, ts);
            break;
        case PropType::Uint64:
            print_tensor<uint64_t, 1>(pval.get<xt::xtensor<uint64_t, 1>>(), ind, ts);
            break;
        case PropType::Bool:
            print_tensor<bool, 1>(pval.get<xt::xtensor<bool, 1>>(), ind, ts);
            break;
        case PropType::Matrix34:
            print_tensor<double, 3>(pval.get<xt::xtensor<double, 3>>(), ind, ts);
            break;
        case PropType::Matrix44:
            print_tensor<double, 3>(pval.get<xt::xtensor<double, 3>>(), ind, ts);
            break;
        case PropType::Vector2:
            print_tensor<double, 2>(pval.get<xt::xtensor<double, 2>>(), ind, ts);
            break;
        case PropType::Vector3:
            print_tensor<double, 2>(pval.get<xt::xtensor<double, 2>>(), ind, ts);
            break;
        case PropType::Vector4:
            print_tensor<double, 2>(pval.get<xt::xtensor<double, 2>>(), ind, ts);
            break;
        default:
            const auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype_name);
            iprint(sf, ERR_MSG_FMT_JSON, msg);
            break;
    }
}

std::vector<std::string> format_pval(PropType ptype, const json& pval)
{
    switch (ptype) {
        case PropType::Bool:
            return {fmt::format("{}", pval.get<bool>())};
        case PropType::String:
            return {fmt::format("\"{:s}\"", pval.get<std::string>())};
        case PropType::Int16:
            return {fmt::format("{}", pval.get<int16_t>())};
        case PropType::Uint16:
            return {fmt::format("{:#06x}", pval.get<uint16_t>())};
        case PropType::Int32:
            return {fmt::format("{}", pval.get<int32_t>())};
        case PropType::Uint32:
            return {fmt::format("{:#010x}", pval.get<uint32_t>())};
        case PropType::Int64:
            return {fmt::format("{}", pval.get<int64_t>())};
        case PropType::Uint64:
            return {fmt::format("{:#018x}", pval.get<uint64_t>())};
        case PropType::Float:
        case PropType::Double:
            return {fmt::format("{}", pval.get<double>())};
        case PropType::Vector2:
        case PropType::Vector3:
        case PropType::Vector4: {
            return format_tensor<double, 1>(pval.get<hvector_t>());
        }
        case PropType::Matrix34:
        case PropType::Matrix44: {
            return format_tensor<double, 2>(pval.get<harray2d_t>());
        }
        default:
            return {};
    }
}

//  Print one property out (do not print PID < 0)
void print_one_prop(const std::string& pname, const json& pval, int pid,
                    const json& verb_props, int verb, int ind, int ts)
{
    const auto sf = ind * ts;
    const auto jverb = g_cfg[j_verbosity];
    const auto verr = jverb[j_error].get<int>();
    const auto vmax = jverb[j_max].get<int>();
    // decode property type
    const auto [basename, ptype_name, ptype, is_array] = parse_prop_name(pname);
    // property verbosity level (if defined) or max
    int pverb;
    // property having an error attached?
    const auto nerr = has_error(pval);

    if (nerr) {
        // the value is an error code, so print it out only if the verbosity
        // is at 'error' level
        pverb = verr;
    }
    else {
        // determine the "active" verbosity level for the current property
        if (verb_props.contains(pname)) {
            // explicitly defined property
            pverb = verb_props[pname].get<int>();
        }
        else {
            // otherwise set requested verbosity to vmax
            pverb = vmax;
        }
    }
    if (verb < pverb) {
        // do not print props which require higher verbosity than the current one
        return;
    }

    // print the prop name
    prop_head_out(pid, basename, is_array, ind, ts);
    if (nerr) {
        const auto msg = get_error_msg(pval);
        fmt::print(ERR_MSG_FMT_JSON, msg);
    }
    else if (is_array) {
        fmt::print("\n");
        print_array_type(pname, pval, ind + 1, ts);
    }
    else {
        const auto fval = format_pval(ptype, pval);
        if (fval.size() == 1) {
            fmt::print("{}\n", fval[0]);
        }
        else if (fval.size() > 1) {
            fmt::print("\n");
            print_multiline(fval, ind + 1, ts);
        }
        else {
            const auto msg = fmt::format(MSG_TYPE_NOT_IMPL, ptype_name);
            fmt::print(ERR_MSG_FMT_JSON, msg);
        }
    }
}

} // namespace basevr
