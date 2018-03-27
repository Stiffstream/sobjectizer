require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.overlimit_redirect_with_filter'

	cpp_source 'main.cpp'
}

