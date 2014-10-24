add_executable(_unit.test.mbox.hanging_subscriptions main.cpp)
target_link_libraries(_unit.test.mbox.hanging_subscriptions so.${SO_5_VERSION})
add_test(_unit.test.mbox.hanging_subscriptions _unit.test.mbox.hanging_subscriptions)
