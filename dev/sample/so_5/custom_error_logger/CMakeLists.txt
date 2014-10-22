add_executable(sample.so_5.custom_error_logger main.cpp)
target_link_libraries(sample.so_5.custom_error_logger so.${SO_5_VERSION})
install(TARGETS sample.so_5.custom_error_logger DESTINATION bin)
