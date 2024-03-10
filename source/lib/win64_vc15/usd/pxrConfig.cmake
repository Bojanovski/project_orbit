# - Configuration file for the pxr project
# Defines the following variables:
# PXR_MAJOR_VERSION - Major version number.
# PXR_MINOR_VERSION - Minor version number.
# PXR_PATCH_VERSION - Patch version number.
# PXR_VERSION       - Complete pxr version string.
# PXR_INCLUDE_DIRS  - Root include directory for the installed project.
# PXR_LIBRARIES     - List of all libraries, by target name.
# PXR_foo_LIBRARY   - Absolute path to individual libraries.
# The preprocessor definition PXR_STATIC will be defined if appropriate

get_filename_component(PXR_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

set(PXR_MAJOR_VERSION "0")
set(PXR_MINOR_VERSION "22")
set(PXR_PATCH_VERSION "11")
set(PXR_VERSION "2211")

# If MaterialX support was enabled for this USD build, try to find the
# associated import targets by invoking the same FindMaterialX.cmake
# module that was used for that build. This can be overridden by
# specifying a different MaterialX_DIR when running cmake.
if(OFF)
    if (NOT DEFINED MaterialX_DIR)
        set(MaterialX_DIR "")
    endif()
    find_package(MaterialX REQUIRED)
endif()

# Similar to MaterialX above, we are using Imath's cmake package config, so set
# the Imath_DIR accordingly to find the associated import targets which were
# used for this USD build. 
# Note that we only need to do this, when it is determined by Imath is being
# used instead of OpenExr (refer Packages.cmake)
if(1)
    if (NOT DEFINED Imath_DIR)
        set(Imath_DIR "C:/db/build/S/VS1564R/Release/imath/lib/cmake/Imath")
    endif()
    find_package(Imath REQUIRED)
endif()

include("${PXR_CMAKE_DIR}/cmake/pxrTargets.cmake")
if (TARGET usd_ms)
    set(libs "usd_ms")
else()
    set(libs "arch;tf;gf;js;trace;work;plug;vt;ar;kind;sdf;ndr;sdr;pcp;usd;usdGeom;usdVol;usdMedia;usdShade;usdLux;usdProc;usdRender;usdHydra;usdRi;usdSkel;usdUI;usdUtils;usdPhysics;garch;hf;hio;cameraUtil;pxOsd;geomUtil;glf;hgi;hgiGL;hgiInterop;hd;hdGp;hdsi;hioOpenVDB;hdSt;hdx;usdImaging;usdImagingGL;usdProcImaging;usdRiImaging;usdSkelImaging;usdVolImaging;usdAppUtils")
endif()
set(PXR_LIBRARIES "")
set(PXR_INCLUDE_DIRS "${PXR_CMAKE_DIR}/include")
string(REPLACE " " ";" libs "${libs}")
foreach(lib ${libs})
    get_target_property(location ${lib} LOCATION)
    set(PXR_${lib}_LIBRARY ${location})
    list(APPEND PXR_LIBRARIES ${lib})
endforeach()
if(NOT ON)
    if(WIN32)
        list(APPEND PXR_LIBRARIES Shlwapi.lib)
        list(APPEND PXR_LIBRARIES Dbghelp.lib)
    endif()
    add_definitions(-DPXR_STATIC)
endif()
