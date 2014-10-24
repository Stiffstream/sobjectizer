add_executable(_unit.test.so_5.svc.simple_lambda_svc main.cpp)
target_link_libraries(_unit.test.so_5.svc.simple_lambda_svc so.${SO_5_VERSION})
add_test(_unit.test.so_5.svc.simple_lambda_svc _unit.test.so_5.svc.simple_lambda_svc)
