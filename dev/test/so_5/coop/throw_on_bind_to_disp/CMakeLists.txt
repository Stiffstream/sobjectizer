add_executable(_unit.test.coop.throw_on_bind_to_disp main.cpp)
target_link_libraries(_unit.test.coop.throw_on_bind_to_disp so.${SO_5_VERSION})
add_test(_unit.test.coop.throw_on_bind_to_disp _unit.test.coop.throw_on_bind_to_disp)
