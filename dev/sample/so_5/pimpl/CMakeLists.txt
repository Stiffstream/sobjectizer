set(SAMPLE sample.so_5.pimpl)
add_executable(${SAMPLE} main.cpp key_value_storage.cpp)
target_link_libraries(${SAMPLE} sobjectizer::SharedLib)
install(TARGETS ${SAMPLE} DESTINATION bin)

set(SAMPLE_S sample.so_5.pimpl_s)
add_executable(${SAMPLE_S} main.cpp key_value_storage.cpp)
target_link_libraries(${SAMPLE_S} sobjectizer::StaticLib)
install(TARGETS ${SAMPLE_S} DESTINATION bin)
