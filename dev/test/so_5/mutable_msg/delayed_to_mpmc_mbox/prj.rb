require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.mutable_msg.delayed_to_mpmc_mbox'

	cpp_source 'main.cpp'
}

