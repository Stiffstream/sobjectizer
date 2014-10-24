add_executable(_unit.test.environment.autoshutdown_in_init main.cpp)
target_link_libraries(_unit.test.environment.autoshutdown_in_init so.${SO_5_VERSION})
add_test(_unit.test.environment.autoshutdown_in_init _unit.test.environment.autoshutdown_in_init)
