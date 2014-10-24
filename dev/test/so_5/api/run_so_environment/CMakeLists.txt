add_executable(_unit.test.api.run_so_environment main.cpp)
target_link_libraries(_unit.test.api.run_so_environment so.${SO_5_VERSION})
add_test(_unit.test.api.run_so_environment _unit.test.api.run_so_environment)
