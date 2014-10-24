add_executable(_unit.test.environment.autoshutdown_disabled main.cpp)
target_link_libraries(_unit.test.environment.autoshutdown_disabled so.${SO_5_VERSION})
add_test(_unit.test.environment.autoshutdown_disabled _unit.test.environment.autoshutdown_disabled)
