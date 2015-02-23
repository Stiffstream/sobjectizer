add_executable(sample.so_5.private_dispatcher_hello main.cpp)
target_link_libraries(sample.so_5.private_dispatcher_hello so.${SO_5_VERSION})
install(TARGETS sample.so_5.private_dispatcher_hello DESTINATION bin)
