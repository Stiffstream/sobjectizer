add_executable(_unit.test.event_handler.subscribe_errors main.cpp)
target_link_libraries(_unit.test.event_handler.subscribe_errors so.${SO_5_VERSION})
add_test(_unit.test.event_handler.subscribe_errors _unit.test.event_handler.subscribe_errors)
