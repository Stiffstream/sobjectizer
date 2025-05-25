include(${CMAKE_SOURCE_DIR}/cmake/sample_as_unittest.cmake)

set(SAMPLE sample.so_5.individual_msg_tracing)
add_executable(${SAMPLE} main.cpp)
target_link_libraries(${SAMPLE} sobjectizer::SharedLib)
install(TARGETS ${SAMPLE} DESTINATION bin)
add_sample_with_shared_lib_as_test(${SAMPLE})

set(SAMPLE_S sample.so_5.individual_msg_tracing_s)
add_executable(${SAMPLE_S} main.cpp)
target_link_libraries(${SAMPLE_S} sobjectizer::StaticLib)
install(TARGETS ${SAMPLE_S} DESTINATION bin)
add_sample_with_static_lib_as_test(${SAMPLE_S})
