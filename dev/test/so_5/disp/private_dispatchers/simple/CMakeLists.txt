add_executable(_unit.test.disp.private_dispatchers.simple main.cpp)
target_link_libraries(_unit.test.disp.private_dispatchers.simple so.${SO_5_VERSION})
add_test(_unit.test.disp.thread_pool.simpl _unit.test.disp.private_dispatchers.simple)
