add_executable(_unit.test.coop.user_resource main.cpp)
target_link_libraries(_unit.test.coop.user_resource so.${SO_5_VERSION})
add_test(_unit.test.coop.user_resource _unit.test.coop.user_resource)
