add_executable(_unit.test.so_5.svc.make_sync_request main.cpp)
target_link_libraries(_unit.test.so_5.svc.make_sync_request so.${SO_5_VERSION})
add_test(_unit.test.so_5.svc.make_sync_request _unit.test.so_5.svc.make_sync_request)
