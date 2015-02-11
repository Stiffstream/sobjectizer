add_executable(sample.so_5.ping_pong_with_owner main.cpp)
target_link_libraries(sample.so_5.ping_pong_with_owner so.${SO_5_VERSION})
install(TARGETS sample.so_5.ping_pong_with_owner DESTINATION bin)
