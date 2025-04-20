# Custom files
# ============
configure_file (
    ${CMAKE_SOURCE_DIR}/res/misc.h.in
    ${CMAKE_BINARY_DIR}/res/misc.h
    )

# Global build defs (proxy target)
# ================================
add_library (build_proxy INTERFACE)
#target_compile_definitions (build_proxy INTERFACE XTENSOR_FORCE_TEMPORARY_MEMORY_IN_ASSIGNMENTS)
target_compile_options (build_proxy INTERFACE /Zc:inline /Zc:__cplusplus /utf-8)
# turn on linker optimization back on in RelWithDebInfo config
target_link_options (build_proxy INTERFACE $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO /OPT:REF /OPT:ICF>)
target_include_directories (build_proxy INTERFACE 
                            "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>"
                            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
                            )
target_include_directories (build_proxy INTERFACE ${CMAKE_BINARY_DIR}/res)

if (DEBUG_INFO)
    target_compile_options (build_proxy /Zi)
    target_link_options (/DEBUG)
endif()

# LibOVR stub (proxy target)
# ==========================
# set the path to OVR_SDK
if (NOT OVR_SDK)
    set (OVR_SDK $ENV{OVR_SDK})
endif()

if (OVR_SDK)
    add_library (LibOVR INTERFACE)
    target_compile_definitions (LibOVR INTERFACE OVR_SDK=${OVR_SDK})
    target_include_directories (LibOVR INTERFACE ${OVR_SDK}/LibOVR/Include)
    target_link_directories (LibOVR INTERFACE ${OVR_SDK}/LibOVR/Lib/Windows/x64/Release/VS2017)
    target_link_libraries (LibOVR INTERFACE LibOVR.lib)
endif()

# Include sub-projects
# ====================
add_subdirectory ("common")
add_subdirectory ("hmdq")
add_subdirectory ("hmdv")

if (BUILD_TESTS)
    add_subdirectory ("hmdq_test")
endif (BUILD_TESTS)

