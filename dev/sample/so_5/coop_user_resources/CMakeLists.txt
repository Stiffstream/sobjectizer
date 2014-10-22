add_executable(sample.so_5.coop_user_resources main.cpp)
target_link_libraries(sample.so_5.coop_user_resources so.${SO_5_VERSION})
install(TARGETS sample.so_5.coop_user_resources DESTINATION bin)
