set(BENCHMARK _test.bench.so_5.coop_dereg)
add_executable(${BENCHMARK} main.cpp)
target_link_libraries(${BENCHMARK} sobjectizer::SharedLib)
