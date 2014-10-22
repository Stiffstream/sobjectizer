add_executable(sample.so_5.hello_world main.cpp)
target_link_libraries(sample.so_5.hello_world so.${SO_5_VERSION})
install(TARGETS sample.so_5.hello_world DESTINATION bin)
