require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.message_limits.subscription_unsubscription_1'

	cpp_source 'main.cpp'
}

