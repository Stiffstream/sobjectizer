add_executable(_unit.test.environment.reg_coop_after_stop main.cpp)
target_link_libraries(_unit.test.environment.reg_coop_after_stop so.${SO_5_VERSION})
add_test(_unit.test.environment.reg_coop_after_stop _unit.test.environment.reg_coop_after_stop)
