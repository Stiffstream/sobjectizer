set(UNITTEST _test.internal_stats.all_dispatchers)
add_executable(${UNITTEST} main.cpp)
target_link_libraries(${UNITTEST} sobjectizer::SharedLib)
