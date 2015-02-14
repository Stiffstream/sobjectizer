add_executable(sample.so_5.watchdog_pimpl main.cpp watchdog.cpp)
target_link_libraries(sample.so_5.watchdog_pimpl so.${SO_5_VERSION})
install(TARGETS sample.so_5.watchdog_pimpl DESTINATION bin)
