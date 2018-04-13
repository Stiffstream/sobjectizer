require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.state.enter_exit_current_state_ptr'

	cpp_source 'main.cpp'
}

