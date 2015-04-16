set(UNITTEST _unit.test.ad_hoc_agents.default_exception_reaction)
add_executable(${UNITTEST} main.cpp)
target_link_libraries(${UNITTEST} so.${SO_5_VERSION})
add_test(${UNITTEST} ${UNITTEST})
