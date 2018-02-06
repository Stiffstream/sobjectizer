require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.event_handler.deadletter_handler_unsubscribe_all_states'

	cpp_source 'main.cpp'
}

