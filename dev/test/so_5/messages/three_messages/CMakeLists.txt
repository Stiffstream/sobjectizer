add_executable(_unit.test.messages.three_messages main.cpp)
target_link_libraries(_unit.test.messages.three_messages so.${SO_5_VERSION})
add_test(_unit.test.messages.three_messages _unit.test.messages.three_messages)
