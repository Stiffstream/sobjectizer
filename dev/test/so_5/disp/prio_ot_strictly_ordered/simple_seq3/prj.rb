require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.disp.prio_ot_strictly_ordered.simple_seq3'

	cpp_source 'main.cpp'
}

