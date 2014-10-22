add_executable(sample.so_5.ping_pong_minimal main.cpp)
target_link_libraries(sample.so_5.ping_pong_minimal so.${SO_5_VERSION})
install(TARGETS sample.so_5.ping_pong_minimal DESTINATION bin)
