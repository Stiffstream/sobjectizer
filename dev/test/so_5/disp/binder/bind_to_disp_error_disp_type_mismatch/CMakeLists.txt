add_executable(_unit.test.disp.binder.bind_to_disp_error_disp_type_mismatch main.cpp)
target_link_libraries(_unit.test.disp.binder.bind_to_disp_error_disp_type_mismatch so.${SO_5_VERSION})
add_test(_unit.test.disp.binder.bind_to_disp_error_disp_type_mismatch _unit.test.disp.binder.bind_to_disp_error_disp_type_mismatch)
