add_executable(_unit.test.coop.duplicate_name main.cpp)
target_link_libraries(_unit.test.coop.duplicate_name so.${SO_5_VERSION})
add_test(_unit.test.coop.duplicate_name _unit.test.coop.duplicate_name)
