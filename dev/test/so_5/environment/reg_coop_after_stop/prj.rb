require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.environment.reg_coop_after_stop" )

	cpp_source( "main.cpp" )
}
