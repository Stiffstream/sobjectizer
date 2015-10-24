require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.messages.user_type_msgs.event_data'

	cpp_source 'main.cpp'
}

