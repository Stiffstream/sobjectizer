require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.mchain.adv_select_mthread_empty_timeout'

	cpp_source 'main.cpp'
}

