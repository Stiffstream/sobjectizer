add_executable(sample.so_5.simple_message_deadline main.cpp)
target_link_libraries(sample.so_5.simple_message_deadline so.${SO_5_VERSION})
install(TARGETS sample.so_5.simple_message_deadline DESTINATION bin)
