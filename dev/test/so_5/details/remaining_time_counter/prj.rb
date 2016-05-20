require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.details.remaining_time_counter'

	cpp_source 'main.cpp'
}

