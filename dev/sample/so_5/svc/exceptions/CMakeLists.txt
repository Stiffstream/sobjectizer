add_executable(sample.so_5.svc.exceptions main.cpp)
target_link_libraries(sample.so_5.svc.exceptions so.${SO_5_VERSION})
install(TARGETS sample.so_5.svc.exceptions DESTINATION bin)
