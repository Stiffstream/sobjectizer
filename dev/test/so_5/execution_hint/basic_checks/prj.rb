require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.execution_hint.basic_checks" )

	cpp_source( "main.cpp" )
}
