add_executable(_unit.test.messages.store_and_resend_later main.cpp)
target_link_libraries(_unit.test.messages.store_and_resend_later so.${SO_5_VERSION})
add_test(_unit.test.messages.store_and_resend_later _unit.test.messages.store_and_resend_later)
