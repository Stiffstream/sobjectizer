add_executable(_unit.test.disp.adv_thread_pool.chained_service_call main.cpp)
target_link_libraries(_unit.test.disp.adv_thread_pool.chained_service_call so.${SO_5_VERSION})
add_test(_unit.test.disp.adv_thread_pool.chained_service_call _unit.test.disp.adv_thread_pool.chained_service_call)
