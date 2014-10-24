add_executable(_unit.test.timer_thread.timers_cancelation main.cpp)
target_link_libraries(_unit.test.timer_thread.timers_cancelation so.${SO_5_VERSION})
add_test(_unit.test.timer_thread.timers_cancelation _unit.test.timer_thread.timers_cancelation)
