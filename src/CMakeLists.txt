add_library(libpdb process.cpp)
add_library(pdb::libpdb ALIAS libpdb)

set_target_properties(
    libpdb
    PROPERTIES OUTPUT_NAME pdb
)

target_compile_features(libpdb PUBLIC cxx_std_17)

target_include_directories(
    libpdb
    PUBLIC 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>    
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/src/include
)

include(GNUInstallDirs)

install(
    TARGETS libpdb
    EXPORT ldb-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT ldb-targets
    FILE pdb-config.cmake
    NAMESPACE pdb::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pdb
)