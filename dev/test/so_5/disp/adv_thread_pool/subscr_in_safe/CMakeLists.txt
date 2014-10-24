add_executable(_unit.test.disp.adv_thread_pool.subscr_in_safe main.cpp)
target_link_libraries(_unit.test.disp.adv_thread_pool.subscr_in_safe so.${SO_5_VERSION})
add_test(_unit.test.disp.adv_thread_pool.subscr_in_safe _unit.test.disp.adv_thread_pool.subscr_in_safe)
