add_executable(sample.so_5.svc.hello main.cpp)
target_link_libraries(sample.so_5.svc.hello so.${SO_5_VERSION})
install(TARGETS sample.so_5.svc.hello DESTINATION bin)
