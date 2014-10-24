add_executable(_unit.test.state.change_state main.cpp)
target_link_libraries(_unit.test.state.change_state so.${SO_5_VERSION})
add_test(_unit.test.state.change_state _unit.test.state.change_state)
