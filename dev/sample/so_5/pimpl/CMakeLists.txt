set(SAMPLE sample.so_5.pimpl)
add_executable(${SAMPLE} main.cpp key_value_storage.cpp)
target_link_libraries(${SAMPLE} so.${SO_5_VERSION})
install(TARGETS ${SAMPLE} DESTINATION bin)

set(SAMPLE_S sample.so_5.pimpl_s)
add_executable(${SAMPLE_S} main.cpp key_value_storage.cpp)
target_link_libraries(${SAMPLE_S} so_s.${SO_5_VERSION})
install(TARGETS ${SAMPLE_S} DESTINATION bin)
