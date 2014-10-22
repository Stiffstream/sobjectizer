add_executable(sample.so_5.exception_reaction main.cpp)
target_link_libraries(sample.so_5.exception_reaction so.${SO_5_VERSION})
install(TARGETS sample.so_5.exception_reaction DESTINATION bin)
