require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.mpsc_queue_traits.locks'

	cpp_source 'main.cpp'
}

