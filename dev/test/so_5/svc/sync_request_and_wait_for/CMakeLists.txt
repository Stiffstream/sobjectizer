add_executable(_unit.test.so_5.svc.sync_request_and_wait_for main.cpp)
target_link_libraries(_unit.test.so_5.svc.sync_request_and_wait_for so.${SO_5_VERSION})
add_test(_unit.test.so_5.svc.sync_request_and_wait_for _unit.test.so_5.svc.sync_request_and_wait_for)
