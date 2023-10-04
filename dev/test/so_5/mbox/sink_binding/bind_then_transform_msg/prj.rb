require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

  required_prj "so_5/prj.rb"

  target "_unit.test.mbox.sink_binding.bind_then_transform_msg"

  cpp_source "impl_no_opt_no_dr.cpp"
  cpp_source "impl_no_opt_with_dr.cpp"
  cpp_source "impl_with_opt_no_dr.cpp"
  cpp_source "impl_with_opt_with_dr.cpp"
  cpp_source "expl_no_opt_no_dr.cpp"
  cpp_source "expl_no_opt_with_dr.cpp"
  cpp_source "expl_with_opt_no_dr.cpp"
  cpp_source "expl_with_opt_with_dr.cpp"
  cpp_source "message_holder_no_opt_no_dr.cpp"
  cpp_source "main.cpp"
}

