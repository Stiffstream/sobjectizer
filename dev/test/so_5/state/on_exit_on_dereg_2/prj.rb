require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.state.on_exit_on_dereg_2'

	cpp_source 'main.cpp'
}

