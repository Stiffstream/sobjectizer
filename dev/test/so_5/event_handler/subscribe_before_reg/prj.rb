require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.event_handler.subscribe_before_reg'

	cpp_source 'main.cpp'
}

