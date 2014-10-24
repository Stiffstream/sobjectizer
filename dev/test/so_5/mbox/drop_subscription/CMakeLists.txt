add_executable(_unit.test.mbox.drop_subscription main.cpp)
target_link_libraries(_unit.test.mbox.drop_subscription so.${SO_5_VERSION})
add_test(_unit.test.mbox.drop_subscription _unit.test.mbox.drop_subscription)
