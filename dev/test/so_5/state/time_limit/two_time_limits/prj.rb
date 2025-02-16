require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.state.time_limit.two_time_limits'

	cpp_source 'main.cpp'
}

