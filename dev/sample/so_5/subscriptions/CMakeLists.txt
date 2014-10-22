add_executable(sample.so_5.subscriptions main.cpp)
target_link_libraries(sample.so_5.subscriptions so.${SO_5_VERSION})
install(TARGETS sample.so_5.subscriptions DESTINATION bin)
