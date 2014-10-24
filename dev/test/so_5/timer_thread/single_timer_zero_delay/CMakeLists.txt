add_executable(_unit.test.timer_thread.single_timer_zero_delay main.cpp)
target_link_libraries(_unit.test.timer_thread.single_timer_zero_delay so.${SO_5_VERSION})
add_test(_unit.test.timer_thread.single_timer_zero_delay _unit.test.timer_thread.single_timer_zero_delay)
