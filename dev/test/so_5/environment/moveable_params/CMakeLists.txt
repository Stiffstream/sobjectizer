add_executable(_unit.test.environment.moveable_params main.cpp)
target_link_libraries(_unit.test.environment.moveable_params so.${SO_5_VERSION})
add_test(_unit.test.environment.moveable_params _unit.test.environment.moveable_params)
