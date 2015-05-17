set(UNITTEST _unit.test.mbox.delivery_filters.filter_on_mpsc_mbox)
add_executable(${UNITTEST} main.cpp)
target_link_libraries(${UNITTEST} so.${SO_5_VERSION})
add_test(${UNITTEST} ${UNITTEST})
