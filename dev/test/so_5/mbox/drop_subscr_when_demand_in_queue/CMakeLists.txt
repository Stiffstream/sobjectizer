add_executable(_unit.test.mbox.drop_subscr_when_demand_in_queue main.cpp)
target_link_libraries(_unit.test.mbox.drop_subscr_when_demand_in_queue so.${SO_5_VERSION})
add_test(_unit.test.mbox.drop_subscr_when_demand_in_queue _unit.test.mbox.drop_subscr_when_demand_in_queue)
