require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.coop.unknown_exception_define_agent'

	cpp_source 'main.cpp'
}

