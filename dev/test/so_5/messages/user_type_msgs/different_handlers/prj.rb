require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.messages.user_type_msgs.different_handlers'

	cpp_source 'main.cpp'
}

