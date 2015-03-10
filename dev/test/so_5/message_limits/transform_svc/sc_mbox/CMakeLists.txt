set(UNITTEST _unit.test.message_limits.transform_svc.sc_mbox)
add_executable(${UNITTEST} main.cpp)
target_link_libraries(${UNITTEST} so.${SO_5_VERSION})
add_test(${UNITTEST} ${UNITTEST})
