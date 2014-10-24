add_executable(_unit.test.event_handler.ignore_exception main.cpp)
target_link_libraries(_unit.test.event_handler.ignore_exception so.${SO_5_VERSION})
add_test(_unit.test.event_handler.ignore_exception _unit.test.event_handler.ignore_exception)
