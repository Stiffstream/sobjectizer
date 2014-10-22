add_executable(sample.so_5.coop_listener main.cpp)
target_link_libraries(sample.so_5.coop_listener so.${SO_5_VERSION})
install(TARGETS sample.so_5.coop_listener DESTINATION bin)
