add_executable(_unit.test.mbox.mpsc_mbox main.cpp)
target_link_libraries(_unit.test.mbox.mpsc_mbox so.${SO_5_VERSION})
add_test(_unit.test.mbox.mpsc_mbox _unit.test.mbox.mpsc_mbox)
