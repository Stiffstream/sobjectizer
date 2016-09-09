require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.internal_stats.simple_work_thread_activity'

	cpp_source 'main.cpp'
}

