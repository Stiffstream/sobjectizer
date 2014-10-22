add_executable(sample.so_5.hello_delay main.cpp)
target_link_libraries(sample.so_5.hello_delay so.${SO_5_VERSION})
install(TARGETS sample.so_5.hello_delay DESTINATION bin)
