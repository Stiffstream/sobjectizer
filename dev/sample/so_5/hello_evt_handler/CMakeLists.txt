add_executable(sample.so_5.hello_evt_handler main.cpp)
target_link_libraries(sample.so_5.hello_evt_handler so.${SO_5_VERSION})
install(TARGETS sample.so_5.hello_evt_handler DESTINATION bin)
