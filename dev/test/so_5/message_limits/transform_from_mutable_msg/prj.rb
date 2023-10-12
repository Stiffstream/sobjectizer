require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.message_limits.transform_from_mutable_msg'

	cpp_source 'main.cpp'
}

