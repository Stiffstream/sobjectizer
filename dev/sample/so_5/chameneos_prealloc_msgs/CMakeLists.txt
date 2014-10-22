add_executable(sample.so_5.chameneos_prealloc_msgs main.cpp)
target_link_libraries(sample.so_5.chameneos_prealloc_msgs so.${SO_5_VERSION})
install(TARGETS sample.so_5.chameneos_prealloc_msgs DESTINATION bin)
