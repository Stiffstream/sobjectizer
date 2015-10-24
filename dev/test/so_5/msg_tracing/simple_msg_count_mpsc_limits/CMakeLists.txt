set(UNITTEST _unit.test.msg_tracing.simple_msg_count_mpsc_limits)
add_executable(${UNITTEST} main.cpp)
target_link_libraries(${UNITTEST} so.${SO_5_VERSION})
add_test(${UNITTEST} ${UNITTEST})
