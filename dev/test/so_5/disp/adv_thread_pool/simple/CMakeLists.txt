add_executable(_unit.test.disp.adv_thread_pool.simple main.cpp)
target_link_libraries(_unit.test.disp.adv_thread_pool.simple so.${SO_5_VERSION})
add_test(_unit.test.disp.adv_thread_pool.simple _unit.test.disp.adv_thread_pool.simple)
