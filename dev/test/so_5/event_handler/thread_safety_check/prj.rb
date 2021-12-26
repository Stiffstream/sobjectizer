require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.event_handler.thread_safety_check'

	cpp_source 'main.cpp'
}

