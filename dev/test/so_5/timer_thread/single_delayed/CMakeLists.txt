add_executable(_unit.test.timer_thread.single_delayed main.cpp)
target_link_libraries(_unit.test.timer_thread.single_delayed so.${SO_5_VERSION})
add_test(_unit.test.timer_thread.single_delayed _unit.test.timer_thread.single_delayed)
