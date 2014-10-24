add_executable(_unit.test.coop.reg_some_and_stop_2 main.cpp)
target_link_libraries(_unit.test.coop.reg_some_and_stop_2 so.${SO_5_VERSION})
add_test(_unit.test.coop.reg_some_and_stop_2 _unit.test.coop.reg_some_and_stop_2)
