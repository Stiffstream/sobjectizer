add_executable(sample.so_5.hello_all main.cpp)
target_link_libraries(sample.so_5.hello_all so.${SO_5_VERSION})
install(TARGETS sample.so_5.hello_all DESTINATION bin)
