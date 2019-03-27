set(BENCHMARK _test.bench.so_5.parallel_parent_child)
add_executable(${BENCHMARK} main.cpp)
target_link_libraries(${BENCHMARK} sobjectizer::SharedLib)
