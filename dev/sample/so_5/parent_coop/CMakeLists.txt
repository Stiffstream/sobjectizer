add_executable(sample.so_5.parent_coop main.cpp)
target_link_libraries(sample.so_5.parent_coop so.${SO_5_VERSION})
install(TARGETS sample.so_5.parent_coop DESTINATION bin)
