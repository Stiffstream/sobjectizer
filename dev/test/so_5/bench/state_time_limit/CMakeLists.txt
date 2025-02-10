set(BENCHMARK _test.bench.so_5.state_time_limit)
add_executable(${BENCHMARK} main.cpp)
target_link_libraries(${BENCHMARK} sobjectizer::SharedLib)
