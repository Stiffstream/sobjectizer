set(SAMPLE sample.so_5.hello_evt_lambda)
add_executable(${SAMPLE} main.cpp)
target_link_libraries(${SAMPLE} so_5::SharedLib)
install(TARGETS ${SAMPLE} DESTINATION bin)

set(SAMPLE_S sample.so_5.hello_evt_lambda_s)
add_executable(${SAMPLE_S} main.cpp)
target_link_libraries(${SAMPLE_S} so_5::StaticLib)
install(TARGETS ${SAMPLE_S} DESTINATION bin)
