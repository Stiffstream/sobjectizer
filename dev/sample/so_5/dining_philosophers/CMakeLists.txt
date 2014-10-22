add_executable(sample.so_5.dining_philosophers main.cpp)
target_link_libraries(sample.so_5.dining_philosophers so.${SO_5_VERSION})
install(TARGETS sample.so_5.dining_philosophers DESTINATION bin)
