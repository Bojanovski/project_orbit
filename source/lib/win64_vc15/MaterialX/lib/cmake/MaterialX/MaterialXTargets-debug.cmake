#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MaterialXCore" for configuration "Debug"
set_property(TARGET MaterialXCore APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MaterialXCore PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/MaterialXCore_d.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/MaterialXCore_d.dll"
  )

list(APPEND _cmake_import_check_targets MaterialXCore )
list(APPEND _cmake_import_check_files_for_MaterialXCore "${_IMPORT_PREFIX}/lib/MaterialXCore_d.lib" "${_IMPORT_PREFIX}/bin/MaterialXCore_d.dll" )

# Import target "MaterialXFormat" for configuration "Debug"
set_property(TARGET MaterialXFormat APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MaterialXFormat PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/MaterialXFormat_d.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/MaterialXFormat_d.dll"
  )

list(APPEND _cmake_import_check_targets MaterialXFormat )
list(APPEND _cmake_import_check_files_for_MaterialXFormat "${_IMPORT_PREFIX}/lib/MaterialXFormat_d.lib" "${_IMPORT_PREFIX}/bin/MaterialXFormat_d.dll" )

# Import target "MaterialXGenShader" for configuration "Debug"
set_property(TARGET MaterialXGenShader APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MaterialXGenShader PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/MaterialXGenShader_d.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/MaterialXGenShader_d.dll"
  )

list(APPEND _cmake_import_check_targets MaterialXGenShader )
list(APPEND _cmake_import_check_files_for_MaterialXGenShader "${_IMPORT_PREFIX}/lib/MaterialXGenShader_d.lib" "${_IMPORT_PREFIX}/bin/MaterialXGenShader_d.dll" )

# Import target "MaterialXGenGlsl" for configuration "Debug"
set_property(TARGET MaterialXGenGlsl APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MaterialXGenGlsl PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/MaterialXGenGlsl_d.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/MaterialXGenGlsl_d.dll"
  )

list(APPEND _cmake_import_check_targets MaterialXGenGlsl )
list(APPEND _cmake_import_check_files_for_MaterialXGenGlsl "${_IMPORT_PREFIX}/lib/MaterialXGenGlsl_d.lib" "${_IMPORT_PREFIX}/bin/MaterialXGenGlsl_d.dll" )

# Import target "MaterialXGenOsl" for configuration "Debug"
set_property(TARGET MaterialXGenOsl APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MaterialXGenOsl PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/MaterialXGenOsl_d.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/MaterialXGenOsl_d.dll"
  )

list(APPEND _cmake_import_check_targets MaterialXGenOsl )
list(APPEND _cmake_import_check_files_for_MaterialXGenOsl "${_IMPORT_PREFIX}/lib/MaterialXGenOsl_d.lib" "${_IMPORT_PREFIX}/bin/MaterialXGenOsl_d.dll" )

# Import target "MaterialXGenMdl" for configuration "Debug"
set_property(TARGET MaterialXGenMdl APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MaterialXGenMdl PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/MaterialXGenMdl_d.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/MaterialXGenMdl_d.dll"
  )

list(APPEND _cmake_import_check_targets MaterialXGenMdl )
list(APPEND _cmake_import_check_files_for_MaterialXGenMdl "${_IMPORT_PREFIX}/lib/MaterialXGenMdl_d.lib" "${_IMPORT_PREFIX}/bin/MaterialXGenMdl_d.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
