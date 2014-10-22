add_executable(sample.so_5.many_timers main.cpp)
target_link_libraries(sample.so_5.many_timers so.${SO_5_VERSION})
install(TARGETS sample.so_5.many_timers DESTINATION bin)
