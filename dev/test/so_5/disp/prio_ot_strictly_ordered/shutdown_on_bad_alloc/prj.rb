require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj_s.rb'

	target '_unit.test.disp.prio_ot_strictly_ordered.shutdown_on_bad_alloc'

	cpp_source 'main.cpp'
}

