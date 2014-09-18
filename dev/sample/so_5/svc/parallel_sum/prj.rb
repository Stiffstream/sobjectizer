require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target 'sample.so_5.svc.parallel_sum'

	cpp_source 'main.cpp'
}

