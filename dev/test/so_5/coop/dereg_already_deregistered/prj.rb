require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.coop.dereg_already_deregistered'

	cpp_source 'main.cpp'
}

