add_executable(_unit.test.mbox.subscribe_when_deregistered main.cpp)
target_link_libraries(_unit.test.mbox.subscribe_when_deregistered so.${SO_5_VERSION})
add_test(_unit.test.mbox.subscribe_when_deregistered _unit.test.mbox.subscribe_when_deregistered)
