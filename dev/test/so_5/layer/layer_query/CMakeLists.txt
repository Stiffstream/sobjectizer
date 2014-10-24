add_executable(_unit.test.layer.layer_query main.cpp)
target_link_libraries(_unit.test.layer.layer_query so.${SO_5_VERSION})
add_test(_unit.test.layer.layer_query _unit.test.layer.layer_query)
