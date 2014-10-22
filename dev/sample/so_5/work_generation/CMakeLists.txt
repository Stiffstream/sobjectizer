add_executable(sample.so_5.work_generation main.cpp)
target_link_libraries(sample.so_5.work_generation so.${SO_5_VERSION})
install(TARGETS sample.so_5.work_generation DESTINATION bin)
