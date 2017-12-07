set(BENCHMARK _test.bench.so_5.prepared_select)
add_executable(${BENCHMARK} main.cpp)
target_link_libraries(${BENCHMARK} sobjectizer::SharedLib)
