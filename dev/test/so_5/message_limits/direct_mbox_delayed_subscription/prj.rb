require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.message_limits.direct_mbox_delayed_subscription'

	cpp_source 'main.cpp'
}

