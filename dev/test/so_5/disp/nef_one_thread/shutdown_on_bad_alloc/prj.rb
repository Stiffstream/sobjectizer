require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj_s.rb'

	target '_unit.test.disp.nef_one_thread.shutdown_on_bad_alloc'

	cpp_source 'main.cpp'
}

