add_executable(sample.so_5.private_dispatcher_for_children main.cpp)
target_link_libraries(sample.so_5.private_dispatcher_for_children so.${SO_5_VERSION})
install(TARGETS sample.so_5.private_dispatcher_for_children DESTINATION bin)
