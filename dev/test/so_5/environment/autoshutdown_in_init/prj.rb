require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.environment.autoshutdown_in_init" )

	cpp_source( "main.cpp" )
}
