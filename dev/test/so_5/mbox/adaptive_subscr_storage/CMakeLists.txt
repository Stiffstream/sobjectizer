add_executable(_unit.test.mbox.adaptive_subscr_storage main.cpp)
target_link_libraries(_unit.test.mbox.adaptive_subscr_storage so.${SO_5_VERSION})
add_test(_unit.test.mbox.adaptive_subscr_storage _unit.test.mbox.adaptive_subscr_storage)
