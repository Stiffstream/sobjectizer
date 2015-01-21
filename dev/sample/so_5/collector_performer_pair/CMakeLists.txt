add_executable(sample.so_5.collector_performer_pair main.cpp)
target_link_libraries(sample.so_5.collector_performer_pair so.${SO_5_VERSION})
install(TARGETS sample.so_5.collector_performer_pair DESTINATION bin)
