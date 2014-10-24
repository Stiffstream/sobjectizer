add_executable(_unit.test.environment.add_disp_after_start main.cpp)
target_link_libraries(_unit.test.environment.add_disp_after_start so.${SO_5_VERSION})
add_test(_unit.test.environment.add_disp_after_start _unit.test.environment.add_disp_after_start)
