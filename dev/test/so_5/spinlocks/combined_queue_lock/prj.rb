require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.spinlocks.combined_queue_lock'

	cpp_source 'main.cpp'
}

