set(UNITTEST _unit.test.disp.prio_ot_quoted_round_robin.simple)
add_executable(${UNITTEST} main.cpp)
target_link_libraries(${UNITTEST} so.${SO_5_VERSION})
add_test(${UNITTEST} ${UNITTEST})
