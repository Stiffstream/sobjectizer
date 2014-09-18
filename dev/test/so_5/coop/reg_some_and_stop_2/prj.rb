require 'mxx_ru/cpp'
MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.coop.reg_some_and_stop_2" )

	cpp_source( "main.cpp" )
}

