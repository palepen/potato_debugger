add_executable(pdb pdb.cpp)
target_link_libraries(pdb PRIVATE pdb::libpdb PkgConfig::readline)
message("tools source directory: ${CMAKE_CURRENT_SOURCE_DIR}")


include(GNUInstallDirs)
install(
    TARGETS pdb
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

