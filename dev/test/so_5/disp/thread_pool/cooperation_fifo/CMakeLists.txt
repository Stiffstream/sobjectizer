add_executable(_unit.test.disp.thread_pool.cooperation_fifo main.cpp)
target_link_libraries(_unit.test.disp.thread_pool.cooperation_fifo so.${SO_5_VERSION})
add_test(_unit.test.disp.thread_pool.cooperation_fifo _unit.test.disp.thread_pool.cooperation_fifo)
