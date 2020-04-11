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

#pragma once

#include <map>
#include <string>

#include <nlohmann/fifo_map.hpp>

#include "prtdef.h"

#include "fifo_map_fix.h"

//  forward declarations
//------------------------------------------------------------------------------
class BaseVRProcessor;
class BaseVRCollector;
class BaseVRConfig;

//  typedefs
//------------------------------------------------------------------------------
// collector buffer
typedef nlohmann::fifo_map<std::string, std::unique_ptr<BaseVRCollector>> colmap_t;
// processor buffer
typedef nlohmann::fifo_map<std::string, std::unique_ptr<BaseVRProcessor>> procmap_t;
// config buffer
typedef nlohmann::fifo_map<std::string, std::unique_ptr<BaseVRConfig>> cfgmap_t;

//  BaseVR class
//------------------------------------------------------------------------------
class BaseVR
{
  protected:
    BaseVR(const char* id, const std::shared_ptr<json>& pjdata)
        : m_id(id), m_pjData(pjdata)
    {}

    // Use virtual destructor to ensure proper object destruction
    virtual ~BaseVR() = default;

  public:
    // Return VR subystem ID
    virtual const std::string& get_id() const final
    {
        return m_id;
    }
    // Return VR subystem data
    virtual std::shared_ptr<json> get_data() final
    {
        return m_pjData;
    }

  protected:
    // Collected data
    std::shared_ptr<json> m_pjData;
    // Subsystem id
    const std::string m_id;
};

//  BaseVRConfig class
//------------------------------------------------------------------------------
class BaseVRConfig : public BaseVR
{
  protected:
    BaseVRConfig(const char* id, const std::shared_ptr<json>& pjdata) : BaseVR(id, pjdata)
    {}
};

//  BaseVRProcessor class
//------------------------------------------------------------------------------
class BaseVRProcessor : public BaseVR
{
  protected:
    BaseVRProcessor(const char* id, const std::shared_ptr<json>& pjdata)
        : BaseVR(id, pjdata)
    {}

  public:
    // Initialize the VR subsystem data processor
    virtual bool init() = 0;
    // Calculate complementary data
    virtual void calculate() = 0;
    // Anonymize sensitive data
    virtual void anonymize() = 0;
    // Print the collected data
    // mode: props, geom, all
    // verb: verbosity
    // ind: indentation
    // ts: indent (tab) size
    virtual void print(pmode mode, int verb, int ind, int ts) const = 0;
    // Clean up the (temporary) data before saving
    virtual void purge() = 0;
};

//  BaseVRCollector class
//------------------------------------------------------------------------------
class BaseVRCollector : public BaseVR
{
  protected:
    BaseVRCollector(const char* id, const std::shared_ptr<json>& pjdata)
        : BaseVR(id, pjdata)
    {}

  public:
    // Check if the VR subsystem is present and initialize it
    // Return: true if present and initialized, otherwise false
    virtual bool try_init() = 0;
    // Collect the VR subsystem data
    virtual void collect() = 0;
    // Return the last VR subsystem error
    virtual int get_last_error() const = 0;
    // Return the last VR subsystem error message
    virtual std::string get_last_error_msg() const = 0;
};
