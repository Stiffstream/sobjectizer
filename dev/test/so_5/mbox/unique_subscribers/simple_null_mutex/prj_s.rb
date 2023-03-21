require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj_s.rb'

	target '_unit.test.so_5.mbox.unique_subscribers.simple_null_mutex_s'

	cpp_source 'main.cpp'
}

