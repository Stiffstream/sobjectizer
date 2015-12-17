require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'
	target 'sample.so_5.producer_consumer_mchain'

	cpp_source 'main.cpp'
}
