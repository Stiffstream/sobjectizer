add_executable(_unit.test.disp.binder.bind_to_disp_error_no_disp main.cpp)
target_link_libraries(_unit.test.disp.binder.bind_to_disp_error_no_disp so.${SO_5_VERSION})
add_test(_unit.test.disp.binder.bind_to_disp_error_no_disp _unit.test.disp.binder.bind_to_disp_error_no_disp)
