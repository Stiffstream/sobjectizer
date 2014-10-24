add_executable(_unit.test.disp.binder.bind_to_disp_3 main.cpp)
target_link_libraries(_unit.test.disp.binder.bind_to_disp_3 so.${SO_5_VERSION})
add_test(_unit.test.disp.binder.bind_to_disp_3 _unit.test.disp.binder.bind_to_disp_3)
