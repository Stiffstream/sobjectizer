add_executable(_test.bench.so_5.ping_pong main.cpp)
target_link_libraries(_test.bench.so_5.ping_pong so.${SO_5_VERSION})
