add_executable(_unit.test.so_5.svc.svc_handler_exception main.cpp)
target_link_libraries(_unit.test.so_5.svc.svc_handler_exception so.${SO_5_VERSION})
add_test(_unit.test.so_5.svc.svc_handler_exception _unit.test.so_5.svc.svc_handler_exception)
