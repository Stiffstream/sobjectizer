add_executable(_unit.test.environment.autoshutdown main.cpp)
target_link_libraries(_unit.test.environment.autoshutdown so.${SO_5_VERSION})
add_test(_unit.test.environment.autoshutdown _unit.test.environment.autoshutdown)
