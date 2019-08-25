#----------------------------------------------------------------------------+
# HMDQ Tools - tools for an OpenVR HMD and other hardware introspection      |
# https://github.com/risa2000/hmdq                                           |
#                                                                            |
# Copyright (c) 2019, Richard Musil. All rights reserved.                    |
#                                                                            |
# This source code is licensed under the BSD 3-Clause "New" or "Revised"     |
# License found in the LICENSE file in the root directory of this project.   |
# SPDX-License-Identifier: BSD-3-Clause                                      |
#----------------------------------------------------------------------------+

cmake_minimum_required (VERSION 3.8)

function (print_variables)
    message (STATUS "print_variables------------------------------------------{")
    message (STATUS "print_variables (\"${ARGV0}\")")
    if (${ARGC})
        set (match ${ARGV0})
    else()
        set (match "^.*$")
    endif()
    get_cmake_property (var_names VARIABLES)
    foreach (var_name ${var_names})
        if (var_name MATCHES ${match})
            message (STATUS "${var_name} = ${${var_name}}")
        endif()
    endforeach()
    message (STATUS "print_variables------------------------------------------}")
endfunction()
