require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.message_limits.any_unspecified_msg_and_state_time_limit'

	cpp_source 'main.cpp'
}

