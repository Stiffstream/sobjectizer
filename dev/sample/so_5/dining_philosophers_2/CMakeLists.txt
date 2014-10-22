add_executable(sample.so_5.dining_philosophers_2 main.cpp)
target_link_libraries(sample.so_5.dining_philosophers_2 so.${SO_5_VERSION})
install(TARGETS sample.so_5.dining_philosophers_2 DESTINATION bin)
