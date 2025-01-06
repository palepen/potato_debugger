#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "pdb::libpdb" for configuration ""
set_property(TARGET pdb::libpdb APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(pdb::libpdb PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libpdb.a"
  )

list(APPEND _cmake_import_check_targets pdb::libpdb )
list(APPEND _cmake_import_check_files_for_pdb::libpdb "${_IMPORT_PREFIX}/lib64/libpdb.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
