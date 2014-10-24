add_executable(_unit.test.coop.throw_on_define_agent main.cpp)
target_link_libraries(_unit.test.coop.throw_on_define_agent so.${SO_5_VERSION})
add_test(_unit.test.coop.throw_on_define_agent _unit.test.coop.throw_on_define_agent)
