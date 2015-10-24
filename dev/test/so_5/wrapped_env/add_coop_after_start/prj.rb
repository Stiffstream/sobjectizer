require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.wrapped_env.add_coop_after_start'

	cpp_source 'main.cpp'
}

