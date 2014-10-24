add_executable(_unit.test.messages.resend_message main.cpp)
target_link_libraries(_unit.test.messages.resend_message so.${SO_5_VERSION})
add_test(_unit.test.messages.resend_message _unit.test.messages.resend_message)
