require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.event_handler.deactivate_agent_evt_finish'

	cpp_source 'main.cpp'
}

