require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.mchain.adv_select_mthread_read'

	cpp_source 'main.cpp'
}

