add_executable(_unit.test.environment.autoname_coop main.cpp)
target_link_libraries(_unit.test.environment.autoname_coop so.${SO_5_VERSION})
add_test(_unit.test.environment.autoname_coop _unit.test.environment.autoname_coop)
