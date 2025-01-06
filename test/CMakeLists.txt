add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE pdb::libpdb Catch2::Catch2WithMain)
