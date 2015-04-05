require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_test.internal_stats.all_dispatchers'

	cpp_source 'main.cpp'
}

