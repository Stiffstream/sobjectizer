add_executable(_unit.test.disp.binder.correct_unbind_after_throw_on_bind main.cpp)
target_link_libraries(_unit.test.disp.binder.correct_unbind_after_throw_on_bind so.${SO_5_VERSION})
add_test(_unit.test.disp.binder.correct_unbind_after_throw_on_bind _unit.test.disp.binder.correct_unbind_after_throw_on_bind)
