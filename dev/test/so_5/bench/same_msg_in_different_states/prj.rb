require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_test.bench.so_5.same_msg_in_different_states'

	cpp_source 'main.cpp'
}
