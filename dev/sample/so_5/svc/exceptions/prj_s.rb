require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj_s.rb'

	target 'sample.so_5.svc.exceptions_s'

	cpp_source 'main.cpp'
}

