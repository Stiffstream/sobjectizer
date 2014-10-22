add_executable(sample.so_5.hardwork_imit main.cpp)
target_link_libraries(sample.so_5.hardwork_imit so.${SO_5_VERSION})
install(TARGETS sample.so_5.hardwork_imit DESTINATION bin)
