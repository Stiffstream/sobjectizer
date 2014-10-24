add_executable(_unit.test.disp.adv_thread_pool.chained_service_call_adhoc main.cpp)
target_link_libraries(_unit.test.disp.adv_thread_pool.chained_service_call_adhoc so.${SO_5_VERSION})
add_test(_unit.test.disp.adv_thread_pool.chained_service_call_adhoc _unit.test.disp.adv_thread_pool.chained_service_call_adhoc)
