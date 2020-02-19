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

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <botan/filters.h>
#include <botan/hash.h>
#include <botan/hex.h>
#include <botan/pipe.h>

#include "config.h"
#include "except.h"
#include "jtools.h"

#include "fifo_map_fix.h"

//  JSON file I/O
//------------------------------------------------------------------------------
//  Read and parse JSON file, return json data.
json read_json(const std::filesystem::path& inpath)
{
    if (!std::filesystem::exists(inpath)) {
        auto msg = fmt::format("File not found: \"{:s}\"", inpath.u8string());
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

//  Crypto functions
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
    const auto chksm = jcopy[CHECKSUM].get<std::string>();
    jcopy.erase(CHECKSUM);
    const auto vchksm = calculate_checksum(jcopy);
    return (chksm == vchksm);
}
