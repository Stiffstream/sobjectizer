set(SAMPLE sample.so_5.pimpl)
add_executable(${SAMPLE} main.cpp key_value_storage.cpp)
target_link_libraries(${SAMPLE} so.${SO_5_VERSION})
