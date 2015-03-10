set(UNITTEST _unit.test.message_limits.redirect_msg_too_deep.sc_mbox)
add_executable(${UNITTEST} main.cpp)
target_link_libraries(${UNITTEST} so.${SO_5_VERSION})
add_test(${UNITTEST} ${UNITTEST})
