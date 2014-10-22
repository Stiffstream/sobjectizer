add_executable(sample.so_5.disp main.cpp)
target_link_libraries(sample.so_5.disp so.${SO_5_VERSION})
install(TARGETS sample.so_5.disp DESTINATION bin)
