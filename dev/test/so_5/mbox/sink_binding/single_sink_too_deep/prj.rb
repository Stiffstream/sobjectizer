require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

  required_prj "so_5/prj.rb"

  target "_unit.test.mbox.sink_binding.single_sink_too_deep"

  cpp_source "main.cpp"
}

