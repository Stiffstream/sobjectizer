require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.coop.coop_disp_binder'

	cpp_source 'main.cpp'
}

