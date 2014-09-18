require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_test.bench.so_5.parallel_send_to_same_mbox'

	cpp_source 'main.cpp'
}
