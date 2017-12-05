require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.state.const_enter_exit_handlers'

	cpp_source 'main.cpp'
}

