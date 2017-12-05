set(SAMPLE sample.so_5.mchain_multi_consumers)
add_executable(${SAMPLE} main.cpp)
target_link_libraries(${SAMPLE} so_5::SharedLib)
install(TARGETS ${SAMPLE} DESTINATION bin)

set(SAMPLE_S sample.so_5.mchain_multi_consumers_s)
add_executable(${SAMPLE_S} main.cpp)
target_link_libraries(${SAMPLE_S} so_5::StaticLib)
install(TARGETS ${SAMPLE_S} DESTINATION bin)

