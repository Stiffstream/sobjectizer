add_executable(_unit.test.execution_hint.basic_checks main.cpp)
target_link_libraries(_unit.test.execution_hint.basic_checks so.${SO_5_VERSION})
add_test(_unit.test.execution_hint.basic_checks _unit.test.execution_hint.basic_checks)
