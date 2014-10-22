add_executable(sample.so_5.svc.parallel_sum main.cpp)
target_link_libraries(sample.so_5.svc.parallel_sum so.${SO_5_VERSION})
install(TARGETS sample.so_5.svc.parallel_sum DESTINATION bin)
