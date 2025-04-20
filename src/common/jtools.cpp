/******************************************************************************
 * HMDQ Tools - tools for VR headsets and other hardware introspection        *
 * https://github.com/risa2000/hmdq                                           *
 *                                                                            *
 * Copyright (c) 2019, Richard Musil. All rights reserved.                    *
 *                                                                            *
 * This source code is licensed under the BSD 3-Clause "New" or "Revised"     *
 * License found in the LICENSE file in the root directory of this project.   *
 * SPDX-License-Identifier: BSD-3-Clause                                      *
 ******************************************************************************/

#include <common/config.h>
#include <common/except.h>
#include <common/jkeys.h>
#include <common/json_proxy.h>
#include <common/jtools.h>
#include <common/wintools.h>

#include <botan/filters.h>
#include <botan/hash.h>
#include <botan/hex.h>
#include <botan/pipe.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

//  JSON file I/O
//------------------------------------------------------------------------------
//  Read and parse JSON file, return json data.
json read_json(const std::filesystem::path& inpath)
{
    if (!std::filesystem::exists(inpath)) {
        auto msg = fmt::format("File not found: \"{:s}\"", path_to_utf8(inpath));
        throw hmdq_error(msg);
    }

    // read JSON data input
    std::ifstream jin(inpath);
    json jdata;
    jin >> jdata;
    jin.close();
    return jdata;
}

// Save JSON data into file with indentation.
void write_json(const std::filesystem::path& outpath, const json& jdata, int indent)
{
    std::ofstream jfo(outpath);
    jfo << jdata.dump(indent);
    jfo.close();
}

//  Anonymize functions
//------------------------------------------------------------------------------
//  Anonymize the message in `in` to `out`
void anonymize(std::vector<char>& out, const std::vector<char>& in)
{
    const auto hash_name = fmt::format("Blake2b({:d})", ANON_BITSIZE);
    std::unique_ptr<Botan::HashFunction> b2b(
        Botan::HashFunction::create_or_throw(hash_name));
    const std::string prefix(ANON_PREFIX);
    b2b->update(reinterpret_cast<const uint8_t*>(&in[0]), std::strlen(&in[0]));
    // the size in chars is cipher size in bytes * 2 for BINHEX encoding
    // plus the anon prefix plus the terminating zero
    const auto anon_size = prefix.length() + ANON_BITSIZE / 8 * 2 + 1;
    if (out.size() < anon_size) {
        // resize out buffer to fit the hash
        out.resize(anon_size);
    }
    const auto hash_bh = Botan::hex_encode(b2b->final());
    std::copy(prefix.begin(), prefix.end(), out.begin());
    std::copy(hash_bh.begin(), hash_bh.end(), out.begin() + prefix.length());
    out[anon_size - 1] = '\0';
}

//  Return (string) property value for given property from properties
inline std::string get_prop_val(const json& jdprops, const std::string& pname)
{
    if (jdprops.contains(pname) && jdprops[pname].is_string())
        return jdprops[pname].get<std::string>();
    else {
        return "";
    }
}

//  Anonymize properties in JSON list
void anonymize_jdprops(json& jdprops, const std::vector<std::string>& anon_prop_names,
                       const std::vector<std::string>& seed_prop_names)
{
    // get the list of properties to hash from the config file
    const std::string anon_prefix(ANON_PREFIX);
    for (const auto pname : anon_prop_names) {
        const auto pval = get_prop_val(jdprops, pname);
        if (pval.size() == 0)
            continue;
        // hash only non-empty strings
        const std::string prefix(pval.c_str(), anon_prefix.size());
        if (prefix != anon_prefix) {
            std::vector<char> msgbuff;
            for (const auto pname2 : seed_prop_names) {
                const auto pval2 = get_prop_val(jdprops, pname2);
                std::copy(pval2.begin(), pval2.end(), std::back_inserter(msgbuff));
            }
            std::copy(pval.begin(), pval.end(), std::back_inserter(msgbuff));
            msgbuff.push_back('\0');
            std::vector<char> buffer;
            anonymize(buffer, msgbuff);
            jdprops[pname] = &buffer[0];
        }
    }
}

//  JSON data manipulation
//------------------------------------------------------------------------------
//  Remove all properties with errors reported from the dict.
void purge_jdprops_errors(json& jdprops)
{
    std::vector<std::string> to_drop;
    for (const auto& [pname, pval] : jdprops.items()) {
        if (has_error(pval)) {
            to_drop.push_back(pname);
        }
    }
    // purge the props with errors
    for (const auto& pname : to_drop) {
        jdprops.erase(pname);
    }
}

//  Add an error message to JSON item
void add_error(json& jd, const char* msg)
{
    jd[ERROR_PREFIX] = msg;
}

//  Add an error message to JSON item
void add_error(json& jd, const std::string& msg)
{
    jd[ERROR_PREFIX] = msg;
}

//  Add an error message to JSON item in an array container
void add_error_array(json& jd, const char* msg)
{
    if (!jd.contains(ERROR_PREFIX)) {
        jd[ERROR_PREFIX] = json::array();
    }
    HMDQ_ASSERT(jd[ERROR_PREFIX].is_array());
    jd[ERROR_PREFIX].push_back(msg);
}

//  Return the error message
std::string get_error_msg(const json& jd)
{
    const auto& jerr = get_error(jd);
    if (jerr.is_string()) {
        return jerr.get<std::string>();
    }
    else if (jerr.is_array()) {
        std::vector<std::string> err_list;
        for (const auto& e : jerr) {
            err_list.push_back(e.get<std::string>());
        }
        return fmt::format("{}", fmt::join(err_list.cbegin(), err_list.cend(), ", "));
    }
    else {
        return fmt::format("Invalid error type {}", jerr.type_name());
    }
}

//  Checksum functions
//------------------------------------------------------------------------------
//  Calculate the hash over the JSON string dump using Blake2b(CHKSUM_BITSIZE).
//  The returned value is an upper case string of a binhex encoded hash.
std::string calculate_checksum(const json& jd)
{
    const auto hash_name = fmt::format("Blake2b({:d})", CHKSUM_BITSIZE);
    Botan::Pipe pipe(new Botan::Hash_Filter(hash_name), new Botan::Hex_Encoder);
    // use the most efficient form for checksum (indent=-1)
    pipe.process_msg(jd.dump());
    return pipe.read_all_as_string();
}

//  Verify the checksum in the JSON dics (if there is one)
bool verify_checksum(const json& jd)
{
    if (!has_checksum(jd))
        return false;

    json jcopy = jd;
    const auto chksm = jcopy[j_checksum].get<std::string>();
    jcopy.erase(j_checksum);
    const auto vchksm = calculate_checksum(jcopy);
    return (chksm == vchksm);
}
