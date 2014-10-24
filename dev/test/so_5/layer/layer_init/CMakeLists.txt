add_executable(_unit.test.layer.layer_init main.cpp)
target_link_libraries(_unit.test.layer.layer_init so.${SO_5_VERSION})
add_test(_unit.test.layer.layer_init _unit.test.layer.layer_init)
