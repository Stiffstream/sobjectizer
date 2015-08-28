require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.disp.prio_dt_one_per_prio.dereg_when_queue_not_empty'

	cpp_source 'main.cpp'
}

