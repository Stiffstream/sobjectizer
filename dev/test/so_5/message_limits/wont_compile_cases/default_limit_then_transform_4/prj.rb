require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.message_limits.wont_compile_cases.default_limit_then_transform_4'

	cpp_source 'main.cpp'
}

