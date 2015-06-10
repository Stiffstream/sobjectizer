require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.details.invoke_noexcept_code'

	cpp_source 'main.cpp'
}

