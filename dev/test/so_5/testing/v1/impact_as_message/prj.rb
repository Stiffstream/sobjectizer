require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.testing.v1.impact_as_message'

	cpp_source 'main.cpp'
}

