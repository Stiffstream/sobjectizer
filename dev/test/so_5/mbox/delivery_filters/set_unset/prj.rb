require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.mbox.delivery_filters.set_unset'

	cpp_source 'main.cpp'
}

