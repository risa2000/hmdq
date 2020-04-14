#----------------------------------------------------------------------------+
# HMDQ Tools - tools for VR headsets and other hardware introspection        |
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

# Find DLL specified in 'dllname' in 'bin' and 'lib' subpaths
function (find_dll var dllname)
    # check first the 'bin' paths
    unset (_dllpath)
    unset (_dllpath CACHE)
    find_program (_dllpath ${dllname})
    if (NOT _dllpath)
        # now check the 'lib' paths
        set (CMAKE_FIND_LIBRARY_SUFFIXES "")
        find_library (_dllpath ${dllname})
        set (CMAKE_FIND_LIBRARY_SUFFIXES .lib)
        if (NOT _dllpath)
            message (SEND_ERROR "find_dll(${var} ${dllname}) cannot find ${dllname}")
        endif()
    endif()
    set (${var} ${_dllpath} PARENT_SCOPE)
    message (STATUS "find_dll(${var} ${dllname}) -> ${_dllpath}")
endfunction()

# Find all DLLs specified after 'var' in 'bin' and 'lib' subpaths
function (find_dlls var)
    foreach (dll IN ITEMS ${ARGN})
        find_dll (dllpath ${dll})
        list (APPEND dllpaths ${dllpath})
    endforeach()
    set (${var} ${dllpaths} PARENT_SCOPE)
endfunction()

# Collect all DLLs for libs specified after 'tgt_name'
#   libs:       are specified without any suffix
#   tgt_name:   is the name of the target which requires the DLLs
#   found_dlls: will be copied into the same folder as the target binary.
function (collect_dlls tgt_name)
    foreach (lib IN ITEMS ${ARGN})
        add_library (${lib}_tgt SHARED IMPORTED)
        find_dll(dllpath "${lib}d.dll")
        set_property (TARGET ${lib}_tgt PROPERTY IMPORTED_LOCATION_DEBUG ${dllpath})
        find_dll(dllpath "${lib}.dll")
        set_property (TARGET ${lib}_tgt PROPERTY IMPORTED_LOCATION_RELEASE ${dllpath})
        set_property (TARGET ${lib}_tgt PROPERTY IMPORTED_LOCATION_RELWITHDEBINFO ${dllpath})
        list (APPEND EXT_LIBS_DLLS "$<TARGET_FILE:${lib}_tgt>")
        install (FILES "$<TARGET_FILE:${lib}_tgt>"
            DESTINATION .
            )
    endforeach()

    add_custom_command (TARGET ${tgt_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${EXT_LIBS_DLLS}
            $<TARGET_FILE_DIR:${tgt_name}>
        )
endfunction()
