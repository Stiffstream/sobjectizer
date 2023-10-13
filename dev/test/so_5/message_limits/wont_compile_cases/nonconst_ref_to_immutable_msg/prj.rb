require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.message_limits.wont_compile_cases.nonconst_ref_to_immutable_msg'

	cpp_source 'main.cpp'
}

