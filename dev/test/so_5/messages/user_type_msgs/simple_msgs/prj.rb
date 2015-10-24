require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.messages.user_type_msgs.simple_msgs'

	cpp_source 'main.cpp'
}

