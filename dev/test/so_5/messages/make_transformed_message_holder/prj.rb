require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.messages.make_transformed_message_holder'

	cpp_source 'main.cpp'
}

