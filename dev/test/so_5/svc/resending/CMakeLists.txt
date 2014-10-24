add_executable(_unit.test.so_5.svc.resending main.cpp)
target_link_libraries(_unit.test.so_5.svc.resending so.${SO_5_VERSION})
add_test(_unit.test.so_5.svc.resending _unit.test.so_5.svc.resending)
