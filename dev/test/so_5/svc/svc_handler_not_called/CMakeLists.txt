add_executable(_unit.test.so_5.svc.svc_handler_not_called main.cpp)
target_link_libraries(_unit.test.so_5.svc.svc_handler_not_called so.${SO_5_VERSION})
add_test(_unit.test.so_5.svc.svc_handler_not_called _unit.test.so_5.svc.svc_handler_not_called)
