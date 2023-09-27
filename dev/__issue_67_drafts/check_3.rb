require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj_s.rb'
	target '__issue_67__check_3'

	cpp_source 'check_3.cpp'
}

