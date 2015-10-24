require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.wrapped_env.external_stop_then_join'

	cpp_source 'main.cpp'
}

