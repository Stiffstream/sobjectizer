require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj_s.rb'
	target '__issue_67__check_1_1'

	cpp_source 'check_1_1.cpp'
}

