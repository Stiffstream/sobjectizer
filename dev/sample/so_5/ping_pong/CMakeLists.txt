add_executable(sample.so_5.ping_pong main.cpp)
target_link_libraries(sample.so_5.ping_pong so.${SO_5_VERSION})
install(TARGETS sample.so_5.ping_pong DESTINATION bin)
