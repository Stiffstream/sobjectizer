set(SAMPLE sample.so_5.hello_all)
add_executable(${SAMPLE} main.cpp)
target_link_libraries(${SAMPLE} so.${SO_5_VERSION})
install(TARGETS ${SAMPLE} DESTINATION bin)

set(SAMPLE_S sample.so_5.hello_all_s)
add_executable(${SAMPLE_S} main.cpp)
target_link_libraries(${SAMPLE_S} so_s.${SO_5_VERSION})
install(TARGETS ${SAMPLE_S} DESTINATION bin)
