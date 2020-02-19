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

#include <string>

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
typedef std::vector<std::unique_ptr<BaseVRCollector>> colbuff_t;
// processor buffer
typedef std::vector<std::unique_ptr<BaseVRProcessor>> procbuff_t;
// config buffer
typedef std::vector<std::unique_ptr<BaseVRConfig>> cfgbuff_t;

//  BaseVR class
//------------------------------------------------------------------------------
class BaseVR
{
  public:
    // Use virtual destructor to ensure proper object destruction
    virtual ~BaseVR() {}

  public:
    // Return VR subystem ID
    virtual std::string get_id() = 0;
    // Return VR subystem data
    virtual json& get_data() = 0;
};

//  BaseVRConfig class
//------------------------------------------------------------------------------
class BaseVRConfig : public BaseVR
{};

//  BaseVRProcessor class
//------------------------------------------------------------------------------
class BaseVRProcessor : public BaseVR
{
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
    virtual void print(pmode mode, int verb, int ind, int ts) = 0;
    // Clean up the (temporary) data before saving
    virtual void purge() = 0;
};

//  BaseVRCollector class
//------------------------------------------------------------------------------
class BaseVRCollector : public BaseVR
{
  public:
    // Check if the VR subsystem is present and initialize it
    // Return: true if present and initialized, otherwise false
    virtual bool try_init() = 0;
    // Collect the VR subsystem data
    virtual void collect() = 0;
    // Return the last VR subsystem error
    virtual int get_last_error() = 0;
    // Return the last VR subsystem error message
    virtual std::string get_last_error_msg() = 0;
};
