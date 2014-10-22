add_executable(sample.so_5.coop_notification main.cpp)
target_link_libraries(sample.so_5.coop_notification so.${SO_5_VERSION})
install(TARGETS sample.so_5.coop_notification DESTINATION bin)
