add_executable(sample.so_5.chstate main.cpp)
target_link_libraries(sample.so_5.chstate so.${SO_5_VERSION})
install(TARGETS sample.so_5.chstate DESTINATION bin)
