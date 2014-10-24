add_executable(_unit.test.layer.extra_layer_errors main.cpp)
target_link_libraries(_unit.test.layer.extra_layer_errors so.${SO_5_VERSION})
add_test(_unit.test.layer.extra_layer_errors _unit.test.layer.extra_layer_errors)
