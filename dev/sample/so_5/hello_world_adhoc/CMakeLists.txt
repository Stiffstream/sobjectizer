add_executable(sample.so_5.hello_world_adhoc main.cpp)
target_link_libraries(sample.so_5.hello_world_adhoc so.${SO_5_VERSION})
install(TARGETS sample.so_5.hello_world_adhoc DESTINATION bin)
