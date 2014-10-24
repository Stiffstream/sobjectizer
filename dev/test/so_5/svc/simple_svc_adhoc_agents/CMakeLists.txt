add_executable(_unit.test.so_5.svc.simple_svc_adhoc_agents main.cpp)
target_link_libraries(_unit.test.so_5.svc.simple_svc_adhoc_agents so.${SO_5_VERSION})
add_test(_unit.test.so_5.svc.simple_svc_adhoc_agents _unit.test.so_5.svc.simple_svc_adhoc_agents)
