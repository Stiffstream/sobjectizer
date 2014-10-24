add_executable(_unit.test.mbox.mpsc_mbox_illegal_subscriber main.cpp)
target_link_libraries(_unit.test.mbox.mpsc_mbox_illegal_subscriber so.${SO_5_VERSION})
add_test(_unit.test.mbox.mpsc_mbox_illegal_subscriber _unit.test.mbox.mpsc_mbox_illegal_subscriber)
