add_executable(_unit.test.timer_thread.single_periodic main.cpp)
target_link_libraries(_unit.test.timer_thread.single_periodic so.${SO_5_VERSION})
add_test(_unit.test.timer_thread.single_periodic _unit.test.timer_thread.single_periodic)
