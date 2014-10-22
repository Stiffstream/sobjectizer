add_executable(sample.so_5.chameneos_simple main.cpp)
target_link_libraries(sample.so_5.chameneos_simple so.${SO_5_VERSION})
install(TARGETS sample.so_5.chameneos_simple DESTINATION bin)
