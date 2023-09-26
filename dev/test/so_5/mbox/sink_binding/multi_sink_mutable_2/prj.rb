require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

   required_prj "so_5/prj.rb"

   target "_unit.test.mbox.sink_binding.multi_sink_mutable_2"

   cpp_source "main.cpp"
}

