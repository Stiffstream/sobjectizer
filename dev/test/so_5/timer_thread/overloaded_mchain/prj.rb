require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.timer_thread.overloaded_mchain" )

	cpp_source( "main.cpp" )
}

