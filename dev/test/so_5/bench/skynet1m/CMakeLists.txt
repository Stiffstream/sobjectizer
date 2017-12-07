set(BENCHMARK _test.bench.so_5.skynet1m)
add_executable(${BENCHMARK} main.cpp)
target_link_libraries(${BENCHMARK} sobjectizer::SharedLib)
