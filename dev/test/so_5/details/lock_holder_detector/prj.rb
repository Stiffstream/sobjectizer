require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.details.lock_holder_detector'

	cpp_source 'main.cpp'
}

