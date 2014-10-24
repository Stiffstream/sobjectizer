add_executable(_unit.test.so_5.svc.no_svc_handlers main.cpp)
target_link_libraries(_unit.test.so_5.svc.no_svc_handlers so.${SO_5_VERSION})
add_test(_unit.test.so_5.svc.no_svc_handlers _unit.test.so_5.svc.no_svc_handlers)
