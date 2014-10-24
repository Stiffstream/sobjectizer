add_executable(_test.so_5.mpsc_mbox_stress main.cpp)
target_link_libraries(_test.so_5.mpsc_mbox_stress so.${SO_5_VERSION})
add_test(_test.so_5.mpsc_mbox_stress _test.so_5.mpsc_mbox_stress)
