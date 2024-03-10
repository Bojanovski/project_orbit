
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was Config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

####################################################################################

include(CMakeFindDependencyMacro)

include(${CMAKE_CURRENT_LIST_DIR}/OpenColorIOTargets.cmake)

include(FindPackageHandleStandardArgs)
set(OpenColorIO_CONFIG ${CMAKE_CURRENT_LIST_FILE})
find_package_handle_standard_args(OpenColorIO CONFIG_MODE)
