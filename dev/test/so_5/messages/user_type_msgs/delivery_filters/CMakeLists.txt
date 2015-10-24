set(UNITTEST _unit.test.messages.user_type_msgs.delivery_filters)
add_executable(${UNITTEST} main.cpp)
target_link_libraries(${UNITTEST} so.${SO_5_VERSION})
add_test(${UNITTEST} ${UNITTEST})
