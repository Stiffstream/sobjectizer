add_executable(_unit.test.so_5.svc.stable_proxy main.cpp)
target_link_libraries(_unit.test.so_5.svc.stable_proxy so.${SO_5_VERSION})
add_test(_unit.test.so_5.svc.stable_proxy _unit.test.so_5.svc.stable_proxy)
