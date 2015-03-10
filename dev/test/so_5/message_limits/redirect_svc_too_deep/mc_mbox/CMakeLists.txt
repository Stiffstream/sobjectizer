set(UNITTEST _unit.test.message_limits.redirect_svc_too_deep.mc_mbox)
add_executable(${UNITTEST} main.cpp)
target_link_libraries(${UNITTEST} so.${SO_5_VERSION})
add_test(${UNITTEST} ${UNITTEST})
