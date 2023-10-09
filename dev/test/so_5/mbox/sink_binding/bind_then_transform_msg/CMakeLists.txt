set(UNITTEST _unit.test.mbox.sink_binding.bind_then_transform_msg)
set(UNITTEST_SRCFILES
  impl_no_opt_no_dr.cpp
  impl_no_opt_with_dr.cpp
  impl_no_opt_with_dr2.cpp
  impl_with_opt_no_dr.cpp
  impl_with_opt_with_dr.cpp
  expl_no_opt_no_dr.cpp
  expl_no_opt_with_dr.cpp
  expl_no_opt_with_dr2.cpp
  expl_with_opt_no_dr.cpp
  expl_with_opt_with_dr.cpp
  message_holder_no_opt_no_dr.cpp
  impl_free_func_no_opt_no_dr.cpp
  impl_free_func_with_opt_no_dr.cpp
  main.cpp
)
include(${CMAKE_SOURCE_DIR}/cmake/unittest.cmake)
