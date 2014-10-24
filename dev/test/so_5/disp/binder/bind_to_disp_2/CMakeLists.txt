add_executable(_unit.test.disp.binder.bind_to_disp_2 main.cpp)
target_link_libraries(_unit.test.disp.binder.bind_to_disp_2 so.${SO_5_VERSION})
add_test(_unit.test.disp.binder.bind_to_disp_2 _unit.test.disp.binder.bind_to_disp_2)
