set(UNITTEST _unit.test.msg_tracing.simple_svc_count_on_exception)
add_executable(${UNITTEST} main.cpp)
target_link_libraries(${UNITTEST} so.${SO_5_VERSION})
add_test(${UNITTEST} ${UNITTEST})
