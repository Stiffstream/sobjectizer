require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.so_5.mbox.unique_subscribers.delivery_filter'

	cpp_source 'main.cpp'
}

