require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.event_handler.skip_demands_on_dereg'

	cpp_source 'main.cpp'
}

