add_executable(sample.so_5.collector_many_performers main.cpp)
target_link_libraries(sample.so_5.collector_many_performers so.${SO_5_VERSION})
install(TARGETS sample.so_5.collector_many_performers DESTINATION bin)
